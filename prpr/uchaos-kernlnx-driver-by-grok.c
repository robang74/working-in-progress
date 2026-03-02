/* 
 * GROK'S 1ST ATTEMPT TO CONVERT UCHAOS IN A LINUX KERNEL DRIVER
 *
I can handle a PoC kernel module that registers as an auxiliary entropy source via the hwrng (hardware RNG) framework in Linux 5.15.x. This way, it mixes jitter-driven chaos into the pool at boot/init time (when entropy is scarce) and can provide ongoing reads if needed. It's not a direct patch to random.c (which would require kernel rebuild), but a loadable module—safer for testing, and aligns with your 3rd-party plan.

I fetched the original uchaos.c and kernel's random.c sources to port the logic. Key tweaks:

- Replaced user-space sched_yield/clock_gettime with kernel ktime_get_ns and cpu_relax for jitter (kernel-safe delays).
- Used a fixed input string (e.g., a kernel version banner or static text) for hashing, since dmesg access at early boot is tricky (ring buffer not fully populated).
- Hash output mixes into the pool via add_device_randomness at module init (boot-time boost).
- Registered as hwrng with a read callback for ongoing entropy (quality set low, as it's jitter-based, not true HW).
- Kept stochastic branches and djb2tum core for chaos.
- Compiled/tested mentally (no real kernel env here), but this should build with `make modules` in kernel tree.

Here's the PoC module code (`uchaos_entropy.c`—place in drivers/char/, add to Makefile/Kconfig):

To build/test:

1. In kernel source (5.15.x): Add to drivers/char/Makefile: `obj-$(CONFIG_UCHAOS_ENTROPY) += uchaos_entropy.o`
2. In Kconfig: Add `config UCHAOS_ENTROPY bool "uchaos auxiliary entropy" depends on HWRNG help "Jitter-based entropy from uchaos for boot."`
3. `make modules` then `insmod uchaos_entropy.ko`
4. Check dmesg for init, /proc/sys/kernel/random/entropy_avail for boost.

If this doesn't fit (e.g., needs more kernel tweaks), you can take it from here—random.c integration would involve adding a call in init_static_randomdata or similar, but module's safer.
*/

/*
 * uchaos_entropy.c - Kernel module for uchaos-based auxiliary entropy source
 * Based on uchaos v0.2.5.3 by robang74, ported for Linux 5.15.x
 * Registers as hwrng, mixes jitter-hash at init for boot entropy.
 * GPLv2
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <linux/hwrng.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/bitops.h>

#define MODULE_NAME "uchaos_entropy"
#define ENTROPY_BUF_SIZE 512  // Mix in blocks
#define DRY_RUNS 31           // From uchaos -r31
#define EXCEPTION_RANGE 3     // From uchaos -d3
#define INPUT_TEXT "Linux kernel 5.15.x boot entropy seed - fixed string for hashing"  // Static input for boot

// Ported from uchaos: rotl64
static inline u64 rotl64(u64 n, u8 c) {
    c &= 63;
    return (n << c) | (n >> ((-c) & 63));
}

// Ported primes from uchaos
static const u8 primes64[10] = {3, 61, 5, 59, 11, 53, 17, 47, 23, 41};

// Ported djb2tum hash with stochastic branches
static u64 djb2tum(const u8 *str, size_t len, u64 seed) {
    u64 hash = seed ? seed : 5381;
    u64 dmn = 1000000000ULL;  // Initial min delta
    u64 dmx = 0;              // Initial max delta
    u64 avg = 0;              // Avg delta
    u32 cpuid = 0;            // Dummy for rdtsc if needed
    u64 prev_ts = ktime_get_ns();

    for (int run = 0; run < DRY_RUNS; run++) {  // Dry runs for stat stabilization
        for (size_t i = 0; i < len; i++) {
            u64 ts = ktime_get_ns();
            u64 delta = ts - prev_ts;
            prev_ts = ts;

            avg = ((avg * 255) + delta) >> 8;  // Update avg

            // Stochastic branches
            if (delta < dmn) {
                dmn = delta;
                hash = rotl64(hash, primes64[run % 10]);
                hash ^= (hash >> 33);
            } else if (delta > dmx) {
                dmx = delta;
                hash = rotl64(hash, 64 - primes64[run % 10]);
                hash *= primes64[(run + 1) % 10];
            } else if (ABS(delta - avg) > EXCEPTION_RANGE) {
                // Exception mode
                hash ^= rotl64(delta, 32);
                hash = (hash ^ (hash >> 33)) * 0xff51afd7ed558ccdULL;
            }

            hash = ((hash << 5) + hash) + str[i];  // djb2 core
            cpu_relax();  // Kernel-safe jitter delay
        }
    }

    return hash;
}

// Generate entropy buffer using hash
static void generate_entropy(u8 *buf, size_t size) {
    u64 seed = ktime_get_ns();
    const u8 *input = (const u8 *)INPUT_TEXT;
    size_t input_len = strlen(INPUT_TEXT);

    for (size_t i = 0; i < size; i += sizeof(u64)) {
        u64 hash = djb2tum(input, input_len, seed);
        memcpy(buf + i, &hash, min(sizeof(u64), size - i));
        seed = hash;  // Chain for next
    }
}

// hwrng read callback
static int uchaos_read(struct hwrng *rng, void *buf, size_t max, bool wait) {
    generate_entropy(buf, min(max, ENTROPY_BUF_SIZE));
    return min(max, ENTROPY_BUF_SIZE);
}

static struct hwrng uchaos_rng = {
    .name = MODULE_NAME,
    .read = uchaos_read,
    .quality = 100,  // Low, as jitter-based (adjust as tested)
};

static int __init uchaos_init(void) {
    int ret;
    u8 entropy_buf[ENTROPY_BUF_SIZE];

    printk(KERN_INFO "%s: Initializing auxiliary entropy source\n", MODULE_NAME);

    // Generate and mix at init for boot entropy
    generate_entropy(entropy_buf, sizeof(entropy_buf));
    add_device_randomness(entropy_buf, sizeof(entropy_buf));

    ret = hwrng_register(&uchaos_rng);
    if (ret) {
        printk(KERN_ERR "%s: Failed to register hwrng: %d\n", MODULE_NAME, ret);
        return ret;
    }

    printk(KERN_INFO "%s: Registered successfully\n", MODULE_NAME);
    return 0;
}

static void __exit uchaos_exit(void) {
    hwrng_unregister(&uchaos_rng);
    printk(KERN_INFO "%s: Unregistered\n", MODULE_NAME);
}

module_init(uchaos_init);
module_exit(uchaos_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Inspired by robang74's uchaos");
MODULE_DESCRIPTION("uchaos jitter-based auxiliary entropy source");

