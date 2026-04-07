/*
 * uchaos_dev.h - Character device for uchaos-based jitter hashing
 *  w/ tail-slayer style DRAM channel hedging for low tail latency
 *
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 or Apache 2.0
 *
 * Modified by x/Grok to use hedged reads on critical state and buffer accesses
 * to reduce DRAM refresh-induced tail latency on sporadic 512-bit hash generation.
 *
 * LICENSE WARNING
 *
 * The original project is/was released under Apache 2.0 license and this file
 * isn't incorported the Laurie Wired tailslayer original code, in particular
 * because the original was a C++ header for userland and this is C for kernel
 * space. It is unclear if the mixing of two sources by x/Grok as AI coder can
 * create a derivative opera or something unique and thus arbitrarly licenseable.
 *
 * For sake of fairness I cite the original author of the tail-slayer DRAM code
 * and I offer the code in a dual-licensing GPLv2 or Apache 2.0, as per user wish.
 *
 * This code adaptation respects the spirit and intent of the original works.
 * Full credit goes to the authors of the original codebases for the core ideas 
 * and/or discoveries.
 *
 * Project: https://github.com/robang74/uchaosys
 * License: GNU Public License v2
 * Author: Roberto A. Foglietta
 *
 * Project: https://github.com/LaurieWired/tailslayer
 * License: Apache License 2.0
 * Author: Laurie Wired
 *
 * x/Grok codebase mixage 2026-04-07
 * url: https://x.com/i/grok/share/01304d88641e41acb03e574c7134cf87
 *
 * Note about tail-slayer: it quicken the 1st access to RAM but for long reading
 * it doesn't work. Moreover, it works as long as the balance between C++ threads
 * signaling and block read time ratio is competitive with the first access to RAM.
 * The benchmark show an access time reduction from 2us to 100ns for a sporadic
 * value/buffer RAM access. This is great but unsustainable for longer non-atomic
 * reading. And in user space the kernel preemption cannot be avoided, as well as
 * scheduler interruptions or IRQ activations. Therefore the "atomic" read is not
 * to take in exact technical terms but "a page of ram, max" (4KB) as per "atomic"
 * chunk size to read. This note has been written by code and README reading, only.
 *
 * About uchaos: it is interesting to note that in the best case this tail-slayer
 * can kill tails jittering from 2uS to 10nS but cannot prevent the scheduler jittering
 * while uChaos is able to operate also with 2+3 bits of entropy. In which the 3 are
 * those fundamental and they are still granted even cutting down the random access time,
 * even all of them. Which is hard to believe could happen in a production machine, anyway.
 *
 * CODEBASE WARNING
 *
 * Never tested, not even compiled. Published as-is for sake of curiosity and memo.
 *
 */

#ifndef UCHAOS_DEV_H
#define UCHAOS_DEV_H

#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/preempt.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/barrier.h>

#define MAX_INPUT_SIZE (1024 << 3)

#define AB  (6)
#define ABL (AB-3)
#define ABN (1<<AB)
#define ABX (ABN-1)
#define ABx ((ABN>>1)-1)
#define ABy ((ABN>>2)-1)
#define ABz ((ABN>>3)-1)

#define dtskew(x) (!x || (x)>>28)
#define ONESEC msecs_to_jiffies(1 << 10)
#define getprmx16(w) (5 + (((w) & ABy) << 1))

#define abs_t(t, x)    ({ t _x = (x); (t)((_x < 0) ? -_x : _x); })
#ifndef min_t
#define min_t(t, x, y) ({ t _x = (x); t _y = (y); (t)((_x < _y) ? _x : _y); })
#endif
#ifndef max_t
#define max_t(t, x, y) ({ t _x = (x); t _y = (y); (t)((_x > _y) ? _x : _y); })
#endif
#define align_t(t, x)  ({ uintptr_t _m = sizeof(t) -1; (typeof(x))(((uintptr_t)(x) + _m) & ~_m); })

#define murmul1 0xff51afd7ed558ccdULL
#define murmul2 0xc4ceb9fe1a85ec53ULL
#define rot1    47
#define rot2    17
#define rot3    13

#define HASHSEED 14695981039346656037ULL
#define HASHSIZE (ABN >> 3)

typedef u64 __attribute__((aligned(HASHSIZE))) archul_t;

#define ABL_ALIGN(x) align_t(archul_t, x)

/* ================================================================
 * TailSlayer-style Hedged Reader for Kernel Space (simplified)
 * ================================================================ */

#define TS_N_REPLICAS     4          // N=4 gives excellent tail reduction on most platforms
#define TS_HUGEPAGE_ORDER 30         // 1 GiB hugepage (same as user-space version)
#define TS_CHANNEL_OFFSET 256        // bytes between replicas (must be multiple of sizeof(T))

static struct {
    void *page;                      // 1GiB hugepage
    archul_t *replicas[TS_N_REPLICAS];
    int cores[TS_N_REPLICAS];
    atomic_t initialized;
} ts_hedger = { .initialized = ATOMIC_INIT(0) };

/* Simple kernel hedged read for a single value */
static inline archul_t ts_hedged_read(size_t logical_idx)
{
    archul_t val = ~0ULL;
    unsigned long best_latency = ~0UL;
    u64 t0, t1;

    preempt_disable();   // critical for low jitter in kernel

    for (int r = 0; r < TS_N_REPLICAS; ++r) {
        archul_t *addr = ts_hedger.replicas[r] + logical_idx;
        t0 = ktime_get_ns();

        archul_t candidate = READ_ONCE(*addr);   // hedged load

        t1 = ktime_get_ns();
        if ((t1 - t0) < best_latency) {
            best_latency = t1 - t0;
            val = candidate;
        }
    }

    preempt_enable();
    return val;
}

/* Initialize hedged memory (call once at module init) */
static inline int ts_hedger_init(void)
{
    struct page *pg;
    void *base;
    int i;

    if (atomic_read(&ts_hedger.initialized))
        return 0;

    pg = alloc_pages(GFP_KERNEL | __GFP_ZERO | __GFP_COMP | __GFP_NOWARN,
                     TS_HUGEPAGE_ORDER);
    if (!pg)
        return -ENOMEM;

    ts_hedger.page = page_address(pg);
    if (!ts_hedger.page) {
        __free_pages(pg, TS_HUGEPAGE_ORDER);
        return -EFAULT;
    }

    base = ts_hedger.page;
    for (i = 0; i < TS_N_REPLICAS; ++i) {
        ts_hedger.replicas[i] = (archul_t*)(base + i * TS_CHANNEL_OFFSET);
    }

    /* Core pinning suggestion - adjust to your isolated cores */
    ts_hedger.cores[0] = 11;
    ts_hedger.cores[1] = 12;
    ts_hedger.cores[2] = 13;
    ts_hedger.cores[3] = 14;

    atomic_set(&ts_hedger.initialized, 1);
    pr_info("uChaos: TailSlayer hedged reader initialized (N=%d)\n", TS_N_REPLICAS);
    return 0;
}

static inline void ts_hedger_free(void)
{
    if (ts_hedger.page) {
        free_pages((unsigned long)ts_hedger.page, TS_HUGEPAGE_ORDER);
        ts_hedger.page = NULL;
    }
    atomic_set(&ts_hedger.initialized, 0);
}

/* ================================================================
 * Original uChaos code with hedged integration
 * ================================================================ */

static archul_t *kbuf = NULL;

static atomic_t loop_failure = ATOMIC_INIT(0);

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
    z = (z ^ ( z <<  ABx  )) * murmux2;
    z =  z ^ ( z >> (ABx+2));
    return z;
}

static archul_t djb2tum(archul_t seed, size_t num)
{
    static unsigned long failure_jiff = 0;
    static archul_t dmx = 0, dmn = -1, mavg = 0, ohs = HASHSEED;

#ifdef _PROVIDE_STATS
    static u64 nexp = 0, evnt = 0, ncl = 0, tcyl = 0, nhsh = 0;
    static archul_t avg = 0, jmn = -1, jmx = 0;
#endif

    volatile int i, j = 0;
    register archul_t ent = 0, hsh = ohs;
    archul_t tns, dlt, dff, ons = 0;
    u8 b0, b1, excp = 0;

#ifdef _CHK_LOOP_FAIL
    if (atomic_read(&loop_failure)) {
        if (time_after(jiffies, failure_jiff + ONESEC)) {
            failure_jiff = 0;
            atomic_set(&loop_failure, 0);
        } else goto enforcedquit;
    }
#endif

    if (seed) hsh ^= seed;

    if (!ons) {
        ons = ktime_get_ns();
        hsh = knuthmx(hsh ^ ons);
    }

    for (i = 0; i < num; i++) {
        if (ent && hsh) ent ^= rotlbit(ent, getprmx16(hsh));

reschedule:
#ifdef _CHK_LOOP_FAIL
        if ((++j) >> 10) {
            failure_jiff = jiffies;
            printk(KERN_ALERT "uChaos: EMERGENCY ABORT - potential infinite reschedule!\n");
            atomic_set(&loop_failure, 1);
            goto enforcedquit;
        }
#endif

        cpu_relax();

        do {
            tns = ktime_get_ns();
            dlt = tns - ons;
            if (!dlt) goto reschedule;
            ons = tns;
        } while (dtskew(dlt));

        if (dmn == -1) { dmn = dlt; goto reschedule; }

        if (dlt < dmn) {
            dff = dmn - dlt; dmn = dlt;
            ent ^= -dff ^ dmn;
        } else if (dlt > dmx) {
            dff = dlt - dmn; dmx = dlt;
            ent ^= dff ^ -dmx;
        } else {
            dff = dlt - dmn;
            ent ^= ~dff ^ dmx;
        }

        if (!dff) { hsh = knuthmx(hsh); goto reschedule; }

        if (dff < min_delta + excp) {
            excp += 4;
        } else {
            if (excp) hsh = murmux3(hsh, ons);
            excp = 0;
        }

        tns  = abs_t(u32, dlt - mavg);
        ent ^= (tns > min_delta ? dlt : ~dlt) << ABz;
        ent ^= (tns > min_delta ? ons : ~ons) << rot3;
        ent  = knuthmx(ent ^ (tns > min_delta ? dff : ~dff));

        b0 = ent & 0x01; b1 = ent & 0x02;
        hsh = (hsh << (4 + (b0 ? b1 : 1))) + (b1 ? -hsh : hsh);
        hsh ^= rotlbit(hsh ^ (tns > min_delta ? tns : ~tns), getprmx16(ent >> 2));

        mavg = ((mavg << 8) - mavg + dlt) >> 8;
    }

enforcedquit:
    ent = hsh;
    hsh = murmux3(hsh, ohs);
    ohs = ent;
    return hsh;
}

/* Modified fill function using hedged reads where possible */
static inline ssize_t _unprotected_interuptible_kbuf_fill(size_t len)
{
    archul_t *p = __builtin_assume_aligned(kbuf, 8);
    size_t sent;

    if (!len) return 0;

    len = ABL_ALIGN(len);
    len = min_t(size_t, len, MAX_INPUT_SIZE);

    for (sent = 0; sent < len; sent += HASHSIZE) {
        if (signal_pending(current))
            break;

        /* Use hedged read for internal state mixing when possible */
        if (atomic_read(&ts_hedger.initialized)) {
            archul_t hedged_state = ts_hedged_read(sent / HASHSIZE % 64);
            *p++ = djb2tum(hedged_state ^ HASHSEED, loop_mult);
        } else {
            *p++ = djb2tum(HASHSEED, loop_mult);
        }
    }

    return sent;
}

static inline void __init4_djb2tum(archul_t *ebuf)
{
    archul_t seed = knuthmx(ktime_get_ns());
    ebuf[0] = djb2tum(seed, loop_mult * init_runs);
    seed = murmux3(ktime_get_ns(), seed);
    ebuf[1] = djb2tum(seed, loop_mult);
    ebuf[2] = djb2tum(0, loop_mult);
    ebuf[3] = djb2tum(0, loop_mult);
}

#endif /* UCHAOS_DEV_H */
