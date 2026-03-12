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
#define DRIVER_VERSION "0.3.8"

#define MAX_INPUT_SIZE (1024 << 3)

// --- Module Parameters ---
static int dry_runs = 7;
module_param(dry_runs, int, 0644);
MODULE_PARM_DESC(dry_runs, "Number of dry runs for stat stabilization (default=7)");

static int exception_range = 3;
module_param(exception_range, int, 0644);
MODULE_PARM_DESC(exception_range, "Jitter delta exception range (default=3)");

static int loop_mult = 7;
module_param(loop_mult, int, 0644);
MODULE_PARM_DESC(loop_mult, "Number of repetitions of the hashing loop (default=7)");

// --- Driver State ---
static int major;
static struct class* uchaos_class = NULL;
static struct device* uchaos_device = NULL;
static DEFINE_MUTEX(uchaos_lock);

static const u8 primes64[20] = {  3, 61,  5, 59, 11, 53, 17, 47, 23, 41,
                                 19, 45, 29, 35, 31, 33, 13, 51,  7, 57 };

static inline u64 rotl64(u64 n, u8 c) {
    c &= 63; return (n << c) | (n >> ((-c) & 63));
}

static u64 current_hash = 0;

#define getprmx(val) (primes64[2 + (val & 0x1f)]) // previous %10 was slower
#define HASH_SEED 14695981039346656037ULL
#define MULTIPLIER 0xff51afd7ed558ccdULL
#define HASHSIZE 8
#define BITSIZE 64
#define LSHIFT 33

static inline u64 djb2tum_core(const u8 *str, size_t len) {
    static u64 avg = 0, dmx = 0, dmn = 1000000000ULL, prev_ts = 0;
    static u64 hash = HASH_SEED;
    u64 ts, delta, diff;

    int i;
    for (i = 0; i < len; i++) {
        if( !prev_ts ) { prev_ts = ktime_get_ns(); cpu_relax(); }
// WARNING:
// this might loop forever, because of a BUG rather than in corner case
repeat:
        do {
            ts = ktime_get_ns();
            delta = ts - prev_ts;
            if( !delta ) cpu_relax();
        } while( !delta );

        // avg * 255 = avg + 256 - avg, faster
        avg = ((avg << 8) - avg + delta) >> 8;

        if (delta < dmn) {
            dmn   = delta;
            hash  = rotl64(hash, getprmx(delta));
            hash ^= (hash >> LSHIFT);
        } else if (delta > dmx) {
            dmx   = delta;
            hash  = rotl64(hash, BITSIZE - getprmx(delta));
            hash *= getprmx((dmn + 1));
        } else {
            diff  = (delta > avg) ? (delta - avg) : (avg - delta);
            if (diff > exception_range) { 
                hash ^= rotl64(delta ^ diff, LSHIFT-1);
                hash  = (hash ^ (hash >> LSHIFT)) * MULTIPLIER;
            } else {
                cpu_relax();
                goto repeat;
            }
        }

        hash = ((hash << 5) + hash) + str[i];
        cpu_relax();
    }
    return hash;
}

// Core uchaos logic
static u64 djb2tum(const u8 *str, size_t len, size_t num) {
    u64 hash;
    int j;

    for (j = 0; j < num; j++) hash = djb2tum_core(str, len);

    return hash;
}

// --- File Operations ---

#define murmul1 0xff51afd7ed558ccdULL
#define murmul2 0xc4ceb9fe1a85ec53ULL
typedef u64 archul_t;

static inline archul_t murmux3(archul_t ks, archul_t p) {
    register archul_t z = ks;
    z =  p ^ (( z >> (LSHIFT-4) ) * murmul1);
    z = (z ^ ( z << (LSHIFT-2) )) * murmul2;
    z =  z ^ ( z >>  LSHIFT    );
    return z;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    u64 output;

    if (len < HASHSIZE) return -EINVAL;

    if (*offset > 0) return 0; // EOF after first read

    mutex_lock(&uchaos_lock);
    output = current_hash;
    current_hash = djb2tum((uint8_t *)&current_hash, HASHSIZE, loop_mult);
    output = murmux3(current_hash, output);
    mutex_unlock(&uchaos_lock);

    if (copy_to_user(buffer, (u8 *)&output, HASHSIZE)) return -EFAULT;

    *offset = HASHSIZE;
    return HASHSIZE;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    u8 *k_buf;
    size_t actual_len = min_t(size_t, len, MAX_INPUT_SIZE);

    k_buf = kmalloc(actual_len, GFP_KERNEL);
    if (!k_buf) return -ENOMEM;

    if (copy_from_user(k_buf, buffer, actual_len)) {
        kfree(k_buf);
        return -EFAULT;
    }

    mutex_lock(&uchaos_lock);
    current_hash = djb2tum(k_buf, actual_len, 1);
    current_hash = djb2tum((const u8 *)&current_hash, HASHSIZE, dry_runs);
    mutex_unlock(&uchaos_lock);

    kfree(k_buf);
    return len;
}

static struct file_operations fops = {
    .read = dev_read,
    .write = dev_write,
};

static int __init uchaos_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) return major;
    
    if(loop_mult < 1) loop_mult = 1;
    if(dry_runs  < 1) dry_runs  = 1;

    uchaos_class = class_create(THIS_MODULE, CLASS_NAME);
    uchaos_device = device_create(uchaos_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

    printk(KERN_INFO "uchaos: Loaded with dry_runs=%d, exception_range=%d\n", dry_runs, exception_range);
    return 0;
}

static void __exit uchaos_exit(void) {
    device_destroy(uchaos_class, MKDEV(major, 0));
    class_unregister(uchaos_class);
    class_destroy(uchaos_class);
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "uchaos: Unloaded\n");
}

module_init(uchaos_init);
module_exit(uchaos_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Roberto A. Foglietta");

