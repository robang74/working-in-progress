/*
 * uchaos_dev.c - Character device for uchaos-based jitter hashing
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com> GPLv2
 *
 * Ported for Linux 5.15.x, usage:
 * echo "seed data" > /dev/uchaos   (Triggers hashing/jitter)
 * cat /dev/uchaos                  (Reads the resulting 64-bit hash)
 *
 * insmod lib/modules/uchaos_dev.ko && dmesg > /dev/uchaos    # to load and init
 * dd if=/dev/uchaos bs=8 count=1 | od -x; done               # to check unicity
 * dd if=/dev/uchaos bs=1k count=1k of=/dev/null              # to check speed
 *
 * dd if=/dev/uchaos bs=1k count=8k | RNG_test.gz.sh stdin64 -tlshow 512K
 *
 *******************************************************************************
 *
 * GEMINI 1ST ATTEMPT TO CONVERT GROK'S DRIVER INTO CHARACTER DEVICE DRIVER
 *
 * Converting the driver from a passive entropy source to a character device
 * (/dev/uchaos) makes it much easier to debug, profile, and validate the "chaos"
 * logic through standard user-space tools.
 *
 * AI refactored the code to remove the hwrng framework and replace it with
 * a standard Linux Character Device. I’ve also added Module Parameters so user
 * can tune the dry runs and exception range at runtime without recompiling.
 *
 * Input source: prpr/uchaos-kernlnx-driver-by-grok.c
 * Purpose of using AI: driver template and makefile quick writing
 *
 * Once upon a time we were copying a generic template from a book or Internet
 * nowadays we ask to a chatbot to create a customised one for our needs and
 * then we take care of filling the template of the relevant code.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/cdev.h>

#define DEVICE_NAME "uchaos"
#define CLASS_NAME  "uchaos_cls"
#define DRIVER_VERSION "0.4.4"

#define MAX_INPUT_SIZE (1024 << 3)

// --- Module Parameters ---
static int dry_runs = 31;
module_param(dry_runs, int, 0644);
MODULE_PARM_DESC(init_runs, " Number of initial runs for stat stabilization (default=7) ");

static int exception_range = 3;
module_param(exception_range, int, 0644);
MODULE_PARM_DESC(min_delta, " Minimum delta otherwise do an extra passage (default=3) ");

static int loop_mult = 1;
module_param(loop_mult, int, 0644);
MODULE_PARM_DESC(loop_mult, " Number of repetitions of the hashing loop (default=1) ");

// --- Driver State ---
static int major;
static struct class* uchaos_class = NULL;
static struct device* uchaos_device = NULL;
static DEFINE_MUTEX(uchaos_lock);

#define AB  (6)
#define ABL (AB-3)        //  2 or  3
#define ABN (1<<AB)       // 32 or 64
#define ABX (ABN-1)       // 31 or 63
#define ABx ((ABN>>1)-1)  // 15 or 31
#define ABy ((ABN>>2)-1)  //  7 or 15
#define ABz ((ABN>>3)-1)  //  3 or  7

#define murmul1 0xff51afd7ed558ccdULL
#define murmul2 0xc4ceb9fe1a85ec53ULL
#define rot1    47
#define rot2    17
#define rot3    13

#define getprmx16(w) (5 + (((w) & ABy) << 1))
#define getprmx(val) (primes64[getprmx16((val))]) // previous %10 was slower
#define HASH_SEED 14695981039346656037ULL
#define MULTIPLIER 0xff51afd7ed558ccdULL
#define HASHSIZE (ABN >> 3)
#define LSHIFT (ABx+2)

typedef u64 archul_t;

static archul_t current_hash = 0;

static const u8 primes64[20] = {  3, 61,  5, 59, 11, 53, 17, 47, 23, 41,
                                 19, 45, 29, 35, 31, 33, 13, 51,  7, 57 };

static inline archul_t rotlbit(archul_t n, u8 c) {
    c &= ABX; return (n << c) | (n >> ((-c) & ABX));
}

static inline archul_t knuthmx(archul_t iw) {
    register archul_t w = iw;
    w  = rotlbit(w, getprmx16(w));
    w *= (w & 1) ? 0x9E3779B9 : 0x045d9f3b;
    w ^= rotlbit(w, (w & 2) ? rot1 : rot2);
    return w;
}

static inline archul_t murmux3(archul_t ks, archul_t p)
{
    register archul_t z = ks;
    z =  p ^ ((z >> (ABx-2)) * murmul1);
    z = (z ^ ( z <<  ABx  )) * murmul2;
    z =  z ^ ( z >> (ABx+2));
    return z;
}

static inline archul_t ent_dstl(archul_t tm_4s_nsec, archul_t dlt, archul_t dff)
{
    static archul_t ent = HASH_SEED * MULTIPLIER;
    ent ^= dlt        << ABz ;       // 1st derivative of time
    ent ^= tm_4s_nsec << rot3;       // current monotonic time
    ent  = knuthmx(ent ^ dff);       // 2nd derivative of time
    return ent;
}

#define dtskew(x) (!x || (x)>>28)    // 2^29 is the biggest 2^n before 1E9

static inline archul_t djb2tum_core(archul_t seed)
{
    static archul_t avg = 0, dmx = 0, dmn = -1, jmn = 1, jmx = 0, javg = 0;
    static archul_t ohs = HASH_SEED;
    register archul_t hsh = ohs;

    archul_t tns, dlt, dff, ent = 0, ons = 0;
    u8 b0, b1, excp = 0;
    int i;

    if( seed ) hsh ^= seed;
    else { ons = ent = 0; }

    for (i = 0; i < 1; i++) {
        if(  ent ) ent ^= rotlbit(ent, getprmx16(hsh));
        if( !ons ) {
            ons = ktime_get_ns();
            hsh = knuthmx(hsh ^ ons);
            cpu_relax();
        }
// -----------------------------------------------------------------------------
// WARNING:
// this might loop forever, because of a BUG rather than in corner case
// -----------------------------------------------------------------------------
reschedule:
        do {
            tns = ktime_get_ns();
            dlt = tns - ons;
            if( !dlt )
                cpu_relax();
            else
                ons = tns;
        } while( dtskew(dlt) );

        if(dmn == -1) { dmn = dlt;                            goto reschedule; }

        if( dlt < dmn ) {
            dff = dmn - dlt; dmn = dlt;
            ent ^= -dff ^ dmn;
            //evnt++;
        } else
        if( dlt > dmx ) {
            dff = dlt - dmn; dmx = dlt;
            ent ^= dff ^ -dmx;
            //evnt++;
        } else {
            dff = dlt - dmn;
            ent ^= ~dff ^ dmx;
        }
        if( !dff ) { hsh = knuthmx(hsh);                      goto reschedule; }

        // avg * 255 = avg + 256 - avg, faster
        // avg = ((avg << 8) - avg + dlt) >> 8;

        // dff is jittering for the exeption manager activation
        if( dff < exception_range + excp ) {
            excp += 4;                   // increasing excp and accounting for dff
            //nexp++;
            //skw = 0;
        } else {
            // Knuth, based on gold section seeded by 1E-3 ~ 1E-4 event idx
            if( excp ) { hsh = murmux3(hsh, ons); } excp = 0;
            // min,max jittering can be ommited
            if( jmn == -1 ) jmn = dff;
            else
            if( dff < jmn ) jmn = dff;
            if( dff > jmx ) jmx = dff;
            // avg calculation can be ommitted
            avg += dlt; javg += dff; //ncl++;
        }

        ent ^= dlt        << ABz ;       // 1st derivative of time
        ent ^= tns        << rot3;       // current monotonic time
        ent  = knuthmx(ent ^ dff);       // 2nd derivative of time

        b0 = ent & 0x01, b1 = ent & 0x02;
        hsh = ( hsh << (4 + (b0 ? b1 : 1)) ) + (b1 ? -hsh : hsh);
        hsh ^= rotlbit(hsh, getprmx16(ent >> 2));

        cpu_relax();
    }

    ent = hsh;                       // forget the entropy mixed in hash
    hsh = murmux3(hsh, ohs);         // whitening the hash before deliver
    ohs = ent;                       // keep the hashing internal state

    return hsh;
}

// Core uchaos logic
static inline archul_t djb2tum(archul_t seed, size_t num)
{
    int j;

    for (j = 0; j < num; j++) current_hash = djb2tum_core(seed);

    return current_hash;
}

// --- File Operations ---

static ssize_t dev_read(struct file *filep, char *buffer, size_t len,
    loff_t *offset)
{
    archul_t output;
    size_t sent = 0;

    if (len < HASHSIZE) return -EINVAL;

    if (*offset > 0) return 0; // EOF after first read

    // Continuous loop to fill the user-requested buffer size
    while (len >= HASHSIZE) {
        // Check for signals to remain non-blocking/interruptible
        if (signal_pending(current))
            break;

        mutex_lock(&uchaos_lock);
        output = djb2tum(0, loop_mult);
        mutex_unlock(&uchaos_lock);

        if (copy_to_user(buffer + sent, (u8 *)&output, HASHSIZE))
            return -EFAULT;

        sent += HASHSIZE;
        len -= HASHSIZE;
    }

    return sent;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len,
    loff_t *offset)
{
    size_t n, nh;
    u8 *k_buf = NULL;
    archul_t *p = (archul_t *)buffer;

    if(len < HASHSIZE) return -EINVAL;
    len = min_t(size_t, len, MAX_INPUT_SIZE);

    k_buf = kmalloc(len, GFP_KERNEL);
    if (!k_buf) return -ENOMEM;

    if (copy_from_user(k_buf, buffer, len)) {
        kfree(k_buf);
        return -EFAULT;
    }

    mutex_lock(&uchaos_lock);
    for(n = 0, nh = len >> 3; n < nh; n++)
        current_hash ^= p[n];
    current_hash = djb2tum(current_hash, dry_runs);
    mutex_unlock(&uchaos_lock);

    kfree(k_buf);
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dev_read,
    .write = dev_write,
};

static int __init uchaos_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) return major;

    if(loop_mult < 1) loop_mult = 1;
    if(dry_runs  < 1) dry_runs  = 1;

    uchaos_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(uchaos_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(uchaos_class);
    }

    uchaos_device = device_create(uchaos_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(uchaos_device)) {
        class_destroy(uchaos_class);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(uchaos_device);
    }

    printk(KERN_INFO "uchaos: Loaded with dry_runs=%d, exception_range=%d\n", dry_runs, exception_range);
    return 0;
}

static void __exit uchaos_exit(void)
{
    device_destroy(uchaos_class, MKDEV(major, 0));
    class_unregister(uchaos_class);
    class_destroy(uchaos_class);
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "uchaos: Unloaded\n");
}

module_init(uchaos_init);
module_exit(uchaos_exit);

MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRIVER_VERSION);
MODULE_AUTHOR("Roberto A. Foglietta <roberto.foglietta@gmail.com>");
MODULE_DESCRIPTION("Stochastic jitter-based chaos stream device");

