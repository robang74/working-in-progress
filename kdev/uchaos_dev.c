/*
 * uchaos_dev.c - Character device for uchaos-based jitter hashing
 * (c) Roberto A. Foglietta <roberto.foglietta@gmail.com> GPLv2
 *
 * Ported for Linux 5.15.x, usage: 
 * echo "seed data" > /dev/uchaos   (Triggers hashing/jitter)
 * cat /dev/uchaos                  (Reads the resulting 64-bit hash)
 * 
 * insmod lib/modules/uchaos_dev.ko && dmesg > /dev/uchaos
 * for i in $(seq 8); do cat /dev/uchaos | od -x; done
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
#define MAX_INPUT_SIZE (1024 << 3)

// --- Module Parameters ---
static int dry_runs = 7; 
module_param(dry_runs, int, 0644);
MODULE_PARM_DESC(dry_runs, "Number of dry runs for stat stabilization (default=7)");

static int exception_range = 3;
module_param(exception_range, int, 0644);
MODULE_PARM_DESC(exception_range, "Jitter delta exception range (default=3)");

// --- Driver State ---
static int major;
static struct class* uchaos_class = NULL;
static struct device* uchaos_device = NULL;
static u64  current_hash = 0x5381; // Initial seed
static bool current_hash_ready = 0;
static DEFINE_MUTEX(uchaos_lock);

static const u8 primes64[10] = {3, 61, 5, 59, 11, 53, 17, 47, 23, 41};

static inline u64 rotl64(u64 n, u8 c) {
    c &= 63; return (n << c) | (n >> ((-c) & 63));
}

// Core uchaos logic
static u64 djb2tum(const u8 *str, size_t len, u64 seed) {
    u64 hash    = seed;
    u64 dmn     = 1000000000ULL;
    u64 dmx     = 0;
    u64 avg     = 0;
    u64 prev_ts = ktime_get_ns();
    int i, run;

    for (run = 0; run < dry_runs; run++) {
        for (i = 0; i < len; i++) {
            u64 ts = ktime_get_ns();
            u64 delta = ts - prev_ts;
            prev_ts = ts;

            avg = ((avg * 255) + delta) >> 8;

            if (delta < dmn) {
                dmn = delta;
                hash = rotl64(hash, primes64[run % 10]);
                hash ^= (hash >> 33);
            } else if (delta > dmx) {
                dmx = delta;
                hash = rotl64(hash, 64 - primes64[run % 10]);
                hash *= primes64[(run + 1) % 10];
            } else {
                u64 diff = (delta > avg) ? (delta - avg) : (avg - delta);
                if (diff > exception_range) {
                    hash ^= rotl64(delta, 32);
                    hash = (hash ^ (hash >> 33)) * 0xff51afd7ed558ccdULL;
                }
            }

            hash = ((hash << 5) + hash) + str[i];
            cpu_relax();
        }
    }
    return hash;
}

// --- File Operations ---

#define HASHSIZE 8
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    char output[HASHSIZE]; // Sufficient for u64 in decimal + \n

    if (*offset > 0) return 0; // EOF after first read

    mutex_lock(&uchaos_lock);
    if( ! current_hash_ready )
        current_hash = djb2tum((uint8_t *)&current_hash, HASHSIZE, current_hash);
    memcpy(output, (uint8_t *)&current_hash, HASHSIZE);
    current_hash_ready = 0;
    mutex_unlock(&uchaos_lock);

    //if (len < HASHSIZE) return -EINVAL;

    if (copy_to_user(buffer, output, HASHSIZE)) return -EFAULT;

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
    current_hash = djb2tum(k_buf, actual_len, current_hash);
    current_hash_ready = 1;
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

