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
 * dd if=/dev/uchaos bs=8k count=1k | RNG_test.gz.sh stdin64 -tlshow 512K
 * Since the driver uses a 8KB buffer to provide reads to userland, bs=8K
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
 *
 * Relevant code source: prpr/uchaos.c
 */

#include <linux/jiffies.h>
#include <linux/version.h>
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
#define DRIVER_VERSION "0.5.2"

#define MAX_INPUT_SIZE (1024 << 3)

// --- Module Parameters ---
static int dry_runs = 7;
module_param(dry_runs, int, 0644);
MODULE_PARM_DESC(init_runs,
    " Number of initial runs for stat stabilization (default=7) ");

static int min_delta = 3;
module_param(min_delta, int, 0644);
MODULE_PARM_DESC(min_delta,
    " Minimum delta otherwise do an extra passage (default=3) ");

static int loop_mult = 1;
module_param(loop_mult, int, 0644);
MODULE_PARM_DESC(loop_mult,
    " Number of repetitions of the hashing loop (default=1) ");

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
#define getprmx(val) (primes64[getprmx16((val))])  // previous %10 was slower
#define HASH_SEED 14695981039346656037ULL
#define MULTIPLIER 0xff51afd7ed558ccdULL
#define HASHSIZE (ABN >> 3)
#define LSHIFT (ABx+2)

typedef u64 volatile archul_t __attribute__((aligned(64)));

static archul_t *kbuf = NULL; // Stack allocation, one char device only

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
#if 0
static inline archul_t ent_dstl(archul_t tm_4s_nsec, archul_t dlt, archul_t dff)
{
    static archul_t ent = HASH_SEED * MULTIPLIER;
    ent ^= dlt        << ABz ;       // 1st derivative of time
    ent ^= tm_4s_nsec << rot3;       // current monotonic time
    ent  = knuthmx(ent ^ dff);       // 2nd derivative of time
    return ent;
}
#endif
#define dtskew(x) (!x || (x)>>28)    // 2^29 is the biggest 2^n before 1E9

#define ONESEC msecs_to_jiffies(1<<10)
static unsigned long failure_jiff = 0;
static bool loop_failure = false;

static inline archul_t djb2tum_core(archul_t seed)
{
    static archul_t dmx = 0, dmn = -1, jmn = -1, jmx = 0; //, avg = 0, javg = 0;
    static archul_t ohs = HASH_SEED;
    register archul_t hsh = ohs;

    archul_t tns, dlt, dff, ent = 0, ons = 0;
    u8 b0, b1, excp = 0;
    int i;

    /*
     * RATIONALE: also flooding the system of printks isn't a good idea, after all.
     * There is not an easy way to fall in this "SYSBUG" but also not an easy way to
     * deal with it because it is not within the coding/logic of this driver's scope. 
     */
    if( loop_failure ) {
        if ( time_after(jiffies, failure_jiff + ONESEC) ) {
            failure_jiff = 0;
            loop_failure = false;
        } else goto enforcedquit;
    }

    if( seed ) hsh ^= seed;
    else { ons = ent = 0; }

    for (i = 0; i < 1; i++) {
        if(  ent ) ent ^= rotlbit(ent, getprmx16(hsh));
        if( !ons ) {
            ons = ktime_get_ns();
            hsh = knuthmx(hsh ^ ons);
            cpu_relax();
        }
/* -----------------------------------------------------------------------------
 * WARNING:
 * it might loop forever, because of a BUG rather than falling in a corner case
 * -------------------------------------------------------------------------- */
reschedule:
        /*
         * RATIONALE: we cannot ignore that in some extreme conditions this code can
         * create a livelock rescheduling for an unlimited number of times. Something
         * exotic like ktime_get_ns() function pointer was corrupted in a way that it
         * returns always the same value. The expectation is 3-12 range of reschedules
         * for each function cold-call. When 1% might require 100x more, performance
         * is halved and it is a degradation of the service but never a lock. Hopefully,
         * we never see this kind of failure in a production system. In critical systems
         * a lock/hack by ktime_get_ns() can cost a disaster, not just low-quality entropy
         * or scarcity. Anyway, when ktime_get_ns() systematically fails much probably
         * other parts of the kernel would create DoS or SysFail in such a way that
         * uChaos will be the least of the issues. Not being a critical one, is enough.
         */
        if( (++i) >> 10 ) { // 2^10 is a large arbitrary value, don't overlook when coding
            failure_jiff = jiffies;
            #define UCWRN "uChaos: EMERGENCY ABORT -"
            printk(KERN_ALERT "%s Detected potential infinite reschedule loop!\n", UCWRN);
            printk(KERN_ALERT "%s loops=%d, kbuf_offset=0x%02lx, jiffies=%lu\n", UCWRN,
                i, (unsigned long)kbuf & 255, jiffies);
            loop_failure = true;
            goto enforcedquit; // TODO: a more drastic way is to unregister the char device
        }
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

        // mavg * 255 = mavg + 256 - mavg, faster
        // mavg = ((mavg << 8) - mavg + dlt) >> 8;

        // dff is jittering for the exeption manager activation
        if( dff < min_delta + excp ) {
            excp += 4;               // increasing excp and accounting for dff
            //nexp++;
        } else {
            // Knuth, based on gold section seeded by 1E-3 ~ 1E-4 event idx
            if( excp ) { hsh = murmux3(hsh, ons); } excp = 0;
            // min,max jittering can be ommited
            if( jmn == -1 ) jmn = dff;
            else
            if( dff < jmn ) jmn = dff;
            if( dff > jmx ) jmx = dff;
            // avg calculation can be ommitted
            // avg += dlt; javg += dff; //ncl++;
        }

        ent ^= dlt        << ABz ;   // 1st derivative of time
        ent ^= tns        << rot3;   // current monotonic time
        ent  = knuthmx(ent ^ dff);   // 2nd derivative of time

        b0 = ent & 0x01, b1 = ent & 0x02;
        hsh = ( hsh << (4 + (b0 ? b1 : 1)) ) + (b1 ? -hsh : hsh);
        hsh ^= rotlbit(hsh, getprmx16(ent >> 2));

        cpu_relax();
    }

    /*
     * RATIONALE: if 'goto enforcedquit' is enforced, the system is probably done
     * and near an imminent collapse but this wouldn't allow to creates a DoS here
     * rather than a soft-degradation of the service quality like doing LCG as RNG.
     */
enforcedquit:
    ent = hsh;                       // forget the entropy mixed in hash
    hsh = murmux3(hsh, ohs);         // whitening the hash before deliver
    ohs = ent;                       // keep the hashing internal state

    return hsh;
}

// Core uchaos logic
static inline archul_t djb2tum(archul_t seed, size_t num)
{
    int j;

    for (j = 0; j < num; j++) seed = djb2tum_core(seed);

    return seed;
}

// --- File Operations ---

static ssize_t dev_read(struct file *filep, char *buffer, size_t len,
    loff_t *offset)
{
    archul_t *p = __builtin_assume_aligned((void *)kbuf, 8);
    size_t sent;

    /*
     * RATIONALE: a single read can provide 8KB of data not more. Doing LCG in
     * a middle of a read() on average means 4KB of low-quality entropy which
     * is a VERY bad but RARE condition of a system wide failure, anyway.
     * Within 512 interactions in LCG mode there is a tiny hope that the sequence
     * would not predictable despite the seed isn't known but "tiny" is about
     * gov size attackers not a common threat. However, refuses to provide more
     * LCG instead of entropy is a sany policy: end the current duty and stop.
     */
    if( loop_failure ) return -ETIMEDOUT;

    len = ( len >> ABL ) << ABL;
    if (len < HASHSIZE) return -EINVAL;
    len = min_t(size_t, len, MAX_INPUT_SIZE);

    mutex_lock(&uchaos_lock);
    
    // Continuous loop to fill the user-requested buffer size
    for (sent = 0; sent < len; sent += HASHSIZE) {
        // Check for signals to remain non-blocking/interruptible
        if (signal_pending(current))
            break;
        *p++ = djb2tum(HASH_SEED, loop_mult);
    }

    if ( !sent )
        sent = -ERESTARTSYS;
    else
    if ( copy_to_user(buffer, (u8 *)kbuf, sent) )
        sent = -EFAULT;

    mutex_unlock(&uchaos_lock);

    return sent;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len,
    loff_t *offset)
{
    int ret = 0;
    size_t n, nh;
    archul_t hash;

    if(len < HASHSIZE) return -EINVAL;
    len = min_t(size_t, len, MAX_INPUT_SIZE);

    if (mutex_lock_interruptible(&uchaos_lock))
        return -ERESTARTSYS;

    if(copy_from_user((u8 *)kbuf, buffer, len)) {
        ret = -EFAULT;
    } else {
        ret = len;
        for(n = 0, nh = len >> ABL; n < nh; hash ^= kbuf[n++]);
        (void)djb2tum(hash, dry_runs);
    }

    mutex_unlock(&uchaos_lock);
    return ret;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dev_read,
    .write = dev_write,
};

#define retnfree(x) { kfree((void *)kbuf); return (x); }

static int __init uchaos_init(void)
{
    if(loop_mult < 1) loop_mult = 1;
    if(dry_runs  < 1) dry_runs  = 1;

    // static *ptr allocation at init on: go or not-go, there is not try
    kbuf = kmalloc(MAX_INPUT_SIZE, GFP_KERNEL);
    if (!kbuf) return -ENOMEM;

    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) retnfree( major );

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    uchaos_class = class_create(CLASS_NAME);
#else
    uchaos_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
    if (IS_ERR(uchaos_class)) {
        
        unregister_chrdev(major, DEVICE_NAME);
        retnfree( PTR_ERR(uchaos_class) );
    }

    uchaos_device = device_create(uchaos_class,
        NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(uchaos_device)) {
        class_destroy(uchaos_class);
        unregister_chrdev(major, DEVICE_NAME);
        retnfree( PTR_ERR(uchaos_device) );
    }

    printk(KERN_INFO "uchaos: loop_mult=%d dry_runs=%d, min_delta=%d kbuf%%64:%lu\n",
        loop_mult, dry_runs, min_delta, (uintptr_t)kbuf & 63);

    return 0;
}

static void __exit uchaos_exit(void)
{
    device_destroy(uchaos_class, MKDEV(major, 0));
    class_unregister(uchaos_class);
    class_destroy(uchaos_class);
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "uchaos: unloaded\n");
}

module_init(uchaos_init);
module_exit(uchaos_exit);

MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRIVER_VERSION);
MODULE_AUTHOR("Roberto A. Foglietta <roberto.foglietta@gmail.com>");
MODULE_DESCRIPTION("Stochastic jitter-based chaos stream device");

