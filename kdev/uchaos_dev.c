/*
 * uchaos_dev.c - Character device for uchaos-based jitter hashing
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2
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

#define MODULE_NAME "uchaos"
#define DEVICE_NAME MODULE_NAME
#define  CLASS_NAME MODULE_NAME"_cls"
#define DRIVER_VERSION "0.5.6"
#define DRIVER_LICENSE "GPL v2"
#define DRIVER_AUTHOR  "Roberto A. Foglietta <roberto.foglietta@gmail.com>"
#define DRIVER_DESCRIPTION "Stochastic scheduler-jitter chaos RNG stream device"

// --- Module Parameters ---

static int badb_init = 0;
module_param(badb_init, int, 0644);
MODULE_PARM_DESC(badb_init,
    " Badboy enforces the kernel RNG unsafe init  ([0]:1)");

static int entr_qlty = 100;
module_param(entr_qlty, int, 0644);
MODULE_PARM_DESC(entr_qlty,
    " Entropy source's quality for the kernel (1:[100]:1000)");

static int min_delta = 3;
module_param(min_delta, int, 0644);
MODULE_PARM_DESC(min_delta,
    " Min. expected variance o/wise extra pass  (1:[3]:255)");

static int init_runs = 7;
module_param(init_runs, int, 0644);
MODULE_PARM_DESC(init_runs,
    " N. init runs as Lyapunov decoherence time (1:[7]:63)");

static int loop_mult = 1;
module_param(loop_mult, int, 0644);
MODULE_PARM_DESC(loop_mult,
    " Nun. of turns before providing the output (1:[1]:7)");

// --- Driver State ---

static int major;
static struct class* uchaos_class = NULL;
static struct device* uchaos_device = NULL;
static DEFINE_MUTEX(uchaos_lock);

// --- Fuctional Definitions ---

#define abs_t(t, x) ({ t __x = (x); (__x < 0) ? -__x : __x; })

#define MAX_INPUT_SIZE (1024 << 3)

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

// --- Fuctional Declarations ---

typedef u64 __attribute__((aligned(HASHSIZE))) archul_t;

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

#define dtskew(x) (!x || (x)>>28)    // 2^29 is the biggest 2^n before 1E9

#define ONESEC msecs_to_jiffies(1<<10)

/*
 * ATOMICITY ON A 1CPU vs SMP SYSTEM: the 'loop_failure' flag is read in many 
 * places, but wrote in one only: 'volatile' is fine, 'atomic_t' is the way.
 * However also failure_jiff requires multi-thread protection, by 'uchaos_lock'.
 * Because 'uchaos_lock' protects writes, reading a 'volatile bool' is safe.
 * The 'volatile' isn't a SMP memory barrier as we expect but each CPU core
 * cache therefore for the most general implementation 'atomic_t' is the way.
 */
static volatile bool loop_failure = false;

static inline archul_t djb2tum(archul_t seed, size_t num)
{
    static unsigned long failure_jiff = 0;
    static archul_t dmx = 0, dmn = -1, mavg = 0, ohs = HASH_SEED;
    register archul_t hsh = ohs;
    volatile int i, j = 0;           // volatile as memory barrier in the loop

    archul_t tns, dlt, dff, ent = 0, ons = 0;
    u8 b0, b1, excp = 0;

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

    for (i = 0; i < num; i++) {
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
        if( (++j) >> 10 ) { // 2^10 is a large arbitrary value, don't overlook when coding
            failure_jiff = jiffies;
            #define UCWRN MODULE_NAME": EMERGENCY ABORT -"
            printk(KERN_ALERT UCWRN" Detected potential infinite reschedule loop!\n");
            printk(KERN_INFO  UCWRN" loops=%d,%d kbuf_offset=0x%02lx, jiffies=%lu\n",
                i, j, (unsigned long)kbuf & 255, jiffies);
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

        // dff is jittering for the exeption manager activation
        if( dff < min_delta + excp ) {
            excp += 4;               // increasing excp and accounting for dff
            //nexp++;
        } else {
            // Knuth, based on gold section seeded by 1E-3 ~ 1E-4 event idx
            if( excp ) { hsh = murmux3(hsh, ons); } excp = 0;
            /*
            // min,max jittering can be ommited
            if( jmn == -1 ) jmn = dff;
            else
            if( dff < jmn ) jmn = dff;
            if( dff > jmx ) jmx = dff;
            // avg calculation can be ommitted
            avg += dlt; javg += dff; //ncl++;
            */
        }

        // Half of the values above and below expected but few around creates
        // uncertanty which affects how ent is calculated but not on average
        // Instead, a trigger by "uncommon" events spawn a ca.10-12bit branch
        // and this detour apports unpredicatbility not just flat-white noise.
        #define MAVG(x) (( tns > min_delta ) ? (x) : ~(x))

        tns  = abs_t(u32, dlt - mavg);
        ent ^= MAVG(dlt)  <<      ABz;       // 1st derivative of time
        ent ^= MAVG(ons)  <<     rot3;      // current monotonic time
        ent  = knuthmx(ent ^ MAVG(dff));   // 2nd derivative of time

        b0 = ent & 0x01, b1 = ent & 0x02;
        hsh = ( hsh << (4 + (b0 ? b1 : 1)) ) + (b1 ? -hsh : hsh);
        hsh ^= rotlbit( hsh ^ MAVG(tns), getprmx16(ent >> 2) );

        // Moving average, where (mavg * 255) = (mavg + 256 - mavg) but faster
        mavg = ((mavg << 8) - mavg + dlt) >> 8;

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

// --- File Operations ---

#define ABL_ALIGN(x) ( ( ( (archul_t)(x) + (1 << ABL) -1 ) >> ABL ) << ABL )

static inline ssize_t _unprotected_interuptible_kbuf_fill(size_t len) {
    archul_t *p = __builtin_assume_aligned(kbuf, 8);
    size_t sent;

    if ( !len ) return 0;

    len = (size_t)ABL_ALIGN( len );
    len = min_t(size_t, len, MAX_INPUT_SIZE);

    // Continuous loop to fill the user-requested buffer size
    for (sent = 0; sent < len; sent += HASHSIZE) {
        // Check for signals to remain non-blocking/interruptible
        if (signal_pending(current))
            break;
        *p++ = djb2tum(HASH_SEED, loop_mult);
    }

    return sent;
}

static ssize_t dev_read(struct file *fp, char *ubuf, size_t len, loff_t *of)
{
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

    if (len < HASHSIZE) return -EINVAL;

    if (mutex_lock_interruptible(&uchaos_lock)) // lock for kbuf ------------ //
        return -ERESTARTSYS;

    sent = _unprotected_interuptible_kbuf_fill(len);
    if ( !sent )
        sent = -ERESTARTSYS;
    else
    if ( copy_to_user(ubuf, (u8 *)kbuf, sent) )
        sent = -EFAULT;

    mutex_unlock(&uchaos_lock); // ------------------------------------------- //

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

    /*
     * RATIONALE: because the access to djb2tum() is blocked at dev_read() after
     * an "infinite loop" failure has been detected, there is no other way to
     * unlock the device than writing 8-bytes at least into the char device.
     * Since uChaos shows, both in kernel and userland spaces, to be able to
     * extract entropy from the micro-chaos conditions it creates with dumb
     * input as well then 8-byte from /dev/zero or echo "ciao bella" is enough
     * to trigger the device and ripristinate its functioning. That's why the
     * printk exists in first place: to inform sysadmin or debugging developers
     * that a certain situation arises and it requires more attention than a
     * simple read again or read later. Because it isn't supposed to happen.
     * Moreover, the timeout of one second does not allow that an automated
     * regular write() like a seeder or a watchdog would do frequently (1ms)
     * can bypass the protection that prevents flooding the system of printks.
     */
    if(copy_from_user((u8 *)kbuf, buffer, len)) {
        ret = -EFAULT;
    } else {
        ret = len;
        for(n = 0, nh = len >> ABL; n < nh; hash ^= (archul_t)kbuf[n++]);
        (void)djb2tum(hash, init_runs);
    }

    mutex_unlock(&uchaos_lock);
    return ret;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dev_read,
    .write = dev_write,
};

#include <linux/random.h>
#include <linux/bitops.h>
#include <linux/hw_random.h>

#ifdef hwrng_register
static int uchaos_read(struct hwrng *rng, void *buf, size_t max, bool wait) {
    mutex_lock(&uchaos_lock);
    max = _unprotected_interuptible_kbuf_fill(max);
    memcpy(buf, kbuf, max);
    mutex_unlock(&uchaos_lock);
    return max;
}
static struct hwrng uchaos_rng = {
    .name = MODULE_NAME,
    .read = uchaos_read,
    .quality = 100,
};
#define retnfree(x) { hwrng_unregister(&uchaos_rng); if(kbufptr) kfree(kbufptr); return (x); }
#else
#define retnfree(x) { if(kbufptr) kfree(kbufptr); return (x); }
#endif

static archul_t *kbufptr = NULL;
typedef void (* credit_entropy_bits_t)(size_t nbits);

static int __init uchaos_init(void)
{
#if defined(_CREDIT_INIT_ADDR) && defined(_STATIC_PRINTK)
  /*
   * hwrng_register() OOPS because a kernel bug despite the backport fix
   * then badboy mode and grep credit_init_bits linux-kernel/System.map
   */
    credit_entropy_bits_t kernel_credit_entropy_bits = (credit_entropy_bits_t)\
        _CREDIT_INIT_ADDR + ((uintptr_t)_printk - _STATIC_PRINTK);
#else
#define kernel_credit_entropy_bits(x)
#endif
    // 256 bit are enough to fullfil the kernel pool
    archul_t entropy_buf[4];

    // Parameters ranges sanitisation min values
    if( !loop_mult ) loop_mult = 1;
    if( !init_runs ) init_runs = 1;
    if( !min_delta ) min_delta = 1;
    if( !entr_qlty ) entr_qlty = 1;
    // Parameters ranges sanitisation max values
    if( loop_mult >>  3 ) loop_mult =    7;
    if( init_runs >>  6 ) init_runs =   63;
    if( min_delta >>  8 ) min_delta =  255;
    if( entr_qlty > 1000) entr_qlty = 1000;
    // Parameters ranges sanitisation bool flags
    badb_init = !!badb_init;

    printk(KERN_INFO MODULE_NAME
        ": Initializing(bb:%d) auxiliary entropy source, quality: %d\n",
            badb_init, entr_qlty);
    entropy_buf[0] = ktime_get_ns();
    entropy_buf[0] = djb2tum(entropy_buf[0], init_runs * loop_mult);
    // by default settings, the previous call with init_runs brings in variance
    entropy_buf[1] = ktime_get_ns();
    entropy_buf[1] = djb2tum(entropy_buf[1], loop_mult);
    // by default settings, further calls with loop_mult have a smaller variance
    entropy_buf[2] = djb2tum(HASH_SEED,      loop_mult);
    entropy_buf[3] = djb2tum(HASH_SEED,      loop_mult);

    /* -------------------------------------------------------------------- */ {
    size_t len = sizeof(entropy_buf);

#ifdef hwrng_register
    int err;
    uchaos_rng.quality = entr_qlty;
    err = hwrng_register(&uchaos_rng); // this OOPS because a kernel bug
    if (err) {
        printk(KERN_ERR MODULE_NAME
            ": Failed to register as hwrng source: %d\n", err);
        return err;
    }
    add_hwgenerator_randomness(entropy_buf, len, len << 3);
#else
    //wait_for_random_bytes();
    printk(KERN_INFO MODULE_NAME
        ": Inject entropy %ld bytes, 1st hash: 0x%016llx\n",
            len, entropy_buf[0]);
    #ifdef add_bootloader_randomness
    add_bootloader_randomness(entropy_buf, len);
    #else
    if(!badb_init) {
        add_hwgenerator_randomness(entropy_buf, len, len << 3);
    } else {
        printk(KERN_INFO MODULE_NAME
            ": Credit entropy function address  : 0x%016lx\n",
                (uintptr_t)kernel_credit_entropy_bits);

                                        // when doing good OOPS &
        add_device_randomness(entropy_buf, len);   // this is the only viable
        kernel_credit_entropy_bits(len << 3);     // then badboy mode init! ;-)
    }
    #endif
#endif
    /* -------------------------------------------------------------------- */ }

    // static *ptr allocation at init on: go or not-go, there is not try
    kbufptr = kzalloc(MAX_INPUT_SIZE + HASHSIZE, GFP_KERNEL);
    if (!kbufptr) retnfree( -ENOMEM );
    kbuf = (archul_t *)ABL_ALIGN( kbufptr );

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

    /*
     * RATIONALE: a char device allows to test the quality of the output (audit)
     * optionally also on a living system in production. Despite this could be a
     * leak thus a possible security problem, a flag in compilation and a signed
     * kernel will prevent anyone to load a module or incorporate code that later
     * can be leveraged for a side attack. After all, every debug facility is keen
     * to provide a larger and porose attack surface. No any news about this.
     * The same concept applies to murmur3() as last whitening passage instead of
     * using a cryptographic state of art function like ChaCha20(). Because murmur3
     * spreads the bits, would not masquerade a poor-quality / low-quantity entropy
     * source. This has been shown in userspace with uchaos.c: appling parkmiller32
     * before murmur3, whitening preserve the grid-bias introduced by PM32 zeroing
     * any doubt that murmur3() can conceal a LCG behind its hashing strength.
     */
    uchaos_device = device_create(uchaos_class,
        NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(uchaos_device)) {
        class_destroy(uchaos_class);
        unregister_chrdev(major, DEVICE_NAME);
        retnfree( PTR_ERR(uchaos_device) );
    }

    printk(KERN_INFO MODULE_NAME
        "loop_mult=%d init_runs=%d, min_delta=%d; kbuf_offset: 0x%02lx\n",
            loop_mult, init_runs, min_delta, (uintptr_t)kbuf & 255);

    return 0;
}

static void __exit uchaos_exit(void)
{
    device_destroy(uchaos_class, MKDEV(major, 0));
    //class_unregister(uchaos_class);
    class_destroy(uchaos_class);
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO MODULE_NAME ": unloaded\n");
    retnfree((void)0);
}

module_init(uchaos_init);
module_exit(uchaos_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_LICENSE(DRIVER_LICENSE);
MODULE_VERSION(DRIVER_VERSION);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);

