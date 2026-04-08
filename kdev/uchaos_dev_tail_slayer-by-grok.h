/*
 * uchaos_dev.h - Character device for uchaos-based jitter hashing
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2
 *
 */

#define MAX_INPUT_SIZE (1024 << 3)

#define AB  (6)
#define ABL (AB-3)        //  2 or  3
#define ABN (1<<AB)       // 32 or 64
#define ABX (ABN-1)       // 31 or 63
#define ABx ((ABN>>1)-1)  // 15 or 31
#define ABy ((ABN>>2)-1)  //  7 or 15
#define ABz ((ABN>>3)-1)  //  3 or  7

#define dtskew(x) (!x || (x)>>28) // 2^29 is the biggest 2^n before 1E9
#define ONESEC msecs_to_jiffies(1 << 10)
#define getprmx16(w) (5 + (((w) & ABy) << 1))

#define abs_t(t, x)    ({ t _x = (x);             (t)((_x < 0) ? -_x : _x); })
#ifndef min_t
#define min_t(t, x, y) ({ t _x = (x); t _y = (y); (t)((_x < _y) ? _x : _y); })
#endif
#ifndef max_t
#define max_t(t, x, y) ({ t _x = (x); t _y = (y); (t)((_x > _y) ? _x : _y); })
#endif
#define align_t(t, x)  ({ uintptr_t _m = sizeof(t) -1; \
                                  (typeof(x))(((uintptr_t)(x) + _m) & ~_m); })

#define murmul1 0xff51afd7ed558ccdULL
#define murmul2 0xc4ceb9fe1a85ec53ULL
#define rot1    47
#define rot2    17
#define rot3    13

#define HASHSEED 14695981039346656037ULL
#define HASHSIZE (ABN >> 3)

typedef u64 __attribute__((aligned(HASHSIZE))) archul_t;

#define ABL_ALIGN(x) align_t(archul_t, x)

static archul_t *kbuf = NULL; // Stack allocation, one char device only

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

static inline archul_t murmux3(archul_t ks, archul_t p) {
    register archul_t z = ks;
    z =  p ^ ((z >> (ABx-2)) * murmul1);
    z = (z ^ ( z <<  ABx  )) * murmul2;
    z =  z ^ ( z >> (ABx+2));
    return z;
}

/*
 * ATOMICITY ON A 1CPU vs SMP SYSTEM: the 'loop_failure' flag is read in many
 * places, but wrote in one only: 'volatile' was fine, 'atomic_t' is the way.
 * However also failure_jiff requires multi-thread protection, by 'uchaos_lock'.
 * Because 'uchaos_lock' protects writes, reading a 'volatile bool' is safe.
 * The 'volatile' isn't a SMP memory barrier as we expect but each CPU core
 * cache therefore for the most general implementation 'atomic_t' is the way.
 */
static atomic_t loop_failure = ATOMIC_INIT(0);

static archul_t djb2tum(archul_t seed, size_t num)
{
    static unsigned long failure_jiff = 0;
    static archul_t dmx = 0, dmn = -1, mavg = 0, ohs = HASHSEED;

#ifdef _PROVIDE_STATS
    static u64 nexp = 0, evnt = 0, ncl = 0, tcyl = 0, nhsh = 0;
    static archul_t avg = 0, jmn = -1, jmx = 0;
#endif
    volatile int i, j = 0; // volatile as current CPU memory barrier in the loop
    register archul_t ent = 0, hsh = ohs; // these two in particular need accel.
    archul_t tns, dlt, dff, ons = 0;
    u8 b0, b1, excp = 0;

#ifdef _CHK_LOOP_FAIL
    /*
     * RATIONALE: also flooding the system of printks isn't a good idea, after all.
     * There is not an easy way to fall in this "SYSBUG" but also not an easy way to
     * deal with it because it is not within the coding/logic of this driver's scope.
     */
    if( atomic_read( &loop_failure ) ) {
        if ( time_after(jiffies, failure_jiff + ONESEC) ) {
            failure_jiff = 0;
            atomic_set(&loop_failure, 0);
        } else goto enforcedquit;
    }
#endif

    if( seed ) hsh ^= seed;
    else { ons = ent = 0; } // useless and gcc ignores, unless ons/ent defined static

    if( !ons ) {
        ons = ktime_get_ns();
        hsh = knuthmx(hsh ^ ons);
    }

    for (i = 0; i < num; i++) {
        if( ent && hsh ) ent ^= rotlbit(ent, getprmx16(hsh));
/* -----------------------------------------------------------------------------
 * WARNING:
 * it might loop forever, because of a BUG rather than falling in a corner case
 * -------------------------------------------------------------------------- */
reschedule:
#ifdef _CHK_LOOP_FAIL
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
        // 2^10 is a large arbitrary value, don't overlook 'arbitrary' when coding
        if( (++j) >> 10 ) {
            failure_jiff = jiffies;
            #define UCWRN MODULE_NAME ": EMERGENCY ABORT - "
            printk(KERN_ALERT UCWRN "potential infinite reschedule loop!\n");
            if(verbosity)
            prtkinfo("Abort w/ loops=%d,%d kbuf_offset=0x%02lx, jiffies=%lu\n",
                i, j, (unsigned long)kbuf & 255, jiffies);
            atomic_set(&loop_failure, 1);
            // A more drastic way would be unregistering the char device
            goto enforcedquit;
        }
#endif
        /*
         * Immediately after the infinite loop check,
         * hashing stuff is starting with a CPU nap!
         */
        cpu_relax();
#ifdef _PROVIDE_STATS
        nexp++;
#endif
        do {
            tns = ktime_get_ns();
            dlt = tns - ons;
            if( !dlt ) {                                      goto reschedule; }
            else ons = tns;
        } while( dtskew(dlt) );

        if(dmn == -1) { dmn = dlt;                            goto reschedule; }

        if( dlt < dmn ) {
            dff = dmn - dlt; dmn = dlt;
            ent ^= -dff ^ dmn;
#ifdef _PROVIDE_STATS
            evnt++;
#endif
        } else
        if( dlt > dmx ) {
            dff = dlt - dmn; dmx = dlt;
            ent ^= dff ^ -dmx;
#ifdef _PROVIDE_STATS
            evnt++;
#endif
        } else {
            dff = dlt - dmn;
            ent ^= ~dff ^ dmx;
        }
        if( !dff ) { hsh = knuthmx(hsh);                      goto reschedule; }

        // dff is jittering for the exeption manager activation
        if( dff < min_delta + excp ) {
            excp += 4;                     // excp is accounting vs dff
#ifdef _PROVIDE_STATS
            nexp++;
#endif
        } else {
            // Knuth, based on gold section seeded by 1E-3 ~ 1E-4 event idx
            if( excp ) { hsh = murmux3(hsh, ons); }
            excp = 0;
#ifdef _PROVIDE_STATS
            if( jmn == -1 ) jmn = dff;
            else
            if( dff < jmn ) jmn = dff;
            if( dff > jmx ) jmx = dff;
            avg += dlt; javg += dff;
            ncl++;
#endif
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

        b0 = ent & 0x01; b1 = ent & 0x02;
        hsh = ( hsh << (4 + (b0 ? b1 : 1)) ) + (b1 ? -hsh : hsh);
        hsh ^= rotlbit( hsh ^ MAVG(tns), getprmx16(ent >> 2) );

        // Moving average, where (mavg * 255) = (mavg + 256 - mavg) but faster
        mavg = ((mavg << 8) - mavg + dlt) >> 8;
    }
#ifdef _PROVIDE_STATS
    nexp -= num;
    tcyl += num;
#endif

    /*
     * RATIONALE: if 'goto enforcedquit' is enforced, the system is probably done
     * and near an imminent collapse but this wouldn't allow to creates a DoS here
     * rather than a soft-degradation of the service quality like doing LCG as RNG.
     */
enforcedquit:
    ent = hsh;                             // forget the entropy mixed in hash
    hsh = murmux3(hsh, ohs);               // whitening the hash before deliver
    ohs = ent;                             // keep the hashing internal state

#ifdef _PROVIDE_STATS
    nhsh++;
#endif
    return hsh;
}

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
        *p++ = djb2tum(HASHSEED, loop_mult);
    }

    return sent;
}

static inline void __init4_djb2tum(archul_t *ebuf) {
    archul_t seed;
    seed = knuthmx(ktime_get_ns());
    ebuf[0] = djb2tum(seed, loop_mult * init_runs);
    // by default settings, the previous call with init_runs brings in variance
    seed = murmux3(ktime_get_ns(), seed);
    ebuf[1] = djb2tum(seed, loop_mult);
    // by default settings, further calls with loop_mult have a smaller variance
    ebuf[2] = djb2tum(0,    loop_mult);
    ebuf[3] = djb2tum(0,    loop_mult);
}
