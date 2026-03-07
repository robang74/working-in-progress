/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 license
 */
#define VERSION "v0.4.5"
/* Quick 2k test: cat uchaos.c  | ./chaos -T 2048 | ent
 * Boot log test: cat dmesg.txt | ./uchaos -S -M2 | ent
 *
 * Compile w/libc:      gcc uchaos.c -O3 --fast-math -Wall -o uchaos -s
 * Compile w/musl: musl-gcc uchaos.c -O3 --fast-math -Wall -o uchaos -s -static
 * Compile option: -D_USE_GET_RTSC, -D_USE_LINUX_RANDOM_H
 *
 * Test with: ent, dieharder, PractRand RNG_test (compiled for Ubuntu 22.04 x64)
 *      drive.google.com/file/d/17ymBcxfO2pA8ET7T4ZxiiO2EYW6_F8Lu/view
 * Qemu test: cd bare-minimal-linux-system; sh start.sh "" bzImage.515x
 *      cp -arf prpr/bin update/common/usr (to add missing binaries)
 * Data production: uctest.sh (shell script for faster production)
 *
 * *****************************************************************************
 *
 * PRESENTATION
 *
 * despite a relatively simple coding, this uchaos binary is able to provide
 * nearly 4MB/s of (pseudo) random numbers generated from a static input (known
 * known and constant in time) by stochastics jitter-drive hashing.
 *
 * Not even a special hash, just a modified version of one of the most ancient
 * text-oriented hashing functions that was written before every theory check
 * (practice shows that it was good for real, not for chance).
 *
 * Contrary to the past experiments, the streams mixing here doesn't even happen
 * because it is a single thread process. And by the way, past experiments were
 * mixing almost known text strings but they could have easily mixed the various
 * /dev/random streams in such a way that even if /dev/random would have been
 * "tampered" by a global attacker of by NSA design, then such mixage would
 * have provided good speed and security.
 *
 * Note: current implementation is a text strings-only PoC, it cannot deal with
 * binary input that has EOB char (\0) in the input data. Quick to change but it
 * is not the point here but djb2tum, possibly.
 *
 * While uchaos.c has been developed as early source for /dev/random leveraging
 * dmesg timings in the boot log, it can works with a simple adaptation (text
 * ending with \0 to binary) also with /dev/random stream as input (source).
 *
 * SPOILER
 *
 * When processing /dev/random output, uchaos acts as a conditioning
 * function. Per Bellare (2006/2014), non-PRF functions can preserve
 * security in specific constructions. The kernel's mixing tolerates
 * this, though uchaos itself is not independently NIST-certified.
 *
 * *****************************************************************************
 *
 * PORPOSE
 *
 * .config - Linux/x86 6.12.71 Kernel Configuration → Kernel hacking
 *
 * CONFIG_WARN_ALL_UNSEEDED_RANDOM:
 *
 * Some parts of the kernel contain bugs relating to their use of cryptographically
 * secure random numbers before it's actually possible to generate those numbers
 * securely. This setting ensures that these flaws don't go unnoticed, by enabling
 * a message, should this ever occur. This will allow people with obscure setups
 * to know when things are going wrong, so that they might contact developers
 * about fixing it.
 *
 * Unfortunately, on some models of some architectures getting a fully seeded CRNG
 * is extremely difficult, and so this can result in dmesg getting spammed for a
 * surprisingly long time. This is really bad from a security perspective, and
 * so architecture maintainers really need to do what they can to get the CRNG
 * seeded sooner after the system is booted. However, since users cannot do
 * anything actionable to address this, by default this option is disabled.
 *
 * Say Y here if you want to receive warnings for all uses of unseeded randomness.
 * This will be of use primarily for those developers interested in improving the
 * security of Linux kernels running on their architecture (or subarchitecture).
 *
 * *****************************************************************************
 *
 * OUTPUT TESTS
 *
 * The first run of commit (#9c8f4f00) passed all the dieharder tests with 49MB.
 *
 * gcc uchaos.c -O3 -Wall -o chaos && strip chaos
 * time cat ../prpr/uchaos.c | ./chaos -T 100000 > uchaos.data.01
 *
 * Tests: 100000, collisions: 0 over 6400000 hashes (0.00%) -- OK
 * Times: running: 19.981 s, hashing: 12.980 s, speed: 320.3 Kh/s
 * Bits in common compared to 50 % avg is 50.0000 % (-0.2 ppm)
 *
 * Collisions here are expected to be 100% for a traditional hash w/fixed input.
 *
 * cat uchaos.data.01 | ent
 * Entropy = 7.999997 bits per byte.
 * Optimum compression would reduce the size of this 51200000 byte file by 0 percent.
 * Chi square distribution for 51200000 samples is 247.48, and randomly would exceed
 * this value 62.04 percent of the times (50% is the ideal value, but 10-90% is fine).
 * Arithmetic mean value of data bytes is 127.5076 (127.5 = random).
 * Monte Carlo value for Pi is 3.140672935 (error 0.03 percent).
 * Serial correlation coefficient is 0.000300 (totally uncorrelated = 0.0).
 *
 * *****************************************************************************
 *
 * TODO LIST
 *
 * Before and after writing in the /dev/random, read avail. entropy from proc
 *
 * Create a set of "bit of entropy per byte" polices, and use #if to compile:
 *   - optimistic 7hw 3vm; or flipcoin 4hw 2vm; or minimal 2hw 1vm.
 *
 **************************************************************************** */

#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define AVGV 127.5
#define E3 1000
#define E6 1000000
#define E9 1000000000
#define MAX_READ_SIZE 4096
#define MAX_COMP_SIZE (MAX_READ_SIZE << 1)
#define ABS(a)    ( ( (a) < 0 )  ? -(a) : (a) )
#define MIN(a,b)  ( ( (a) < (b) ) ? (a) : (b) )
#define MAX(a,b)  ( ( (a) > (b) ) ? (a) : (b) )
#define ALGN64(n) ( ( ( (n) + 63) >> 6 ) << 6 )
#define BIT(v,n)  ( ( (v) >> (n) ) & 1 )
#define perr(x...) fprintf(stderr, x)

#define ALPH64 "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz0123456789@\n"

static inline uint8_t *bin2s64(uint8_t *buf, uint32_t nmax) {
    static const uint8_t c[] = ALPH64;
    for(register uint32_t i = 0; i < nmax; i++)
        buf[i] = c[ 0x3F & buf[i] ];
    return buf;
}

#ifdef _USE_GET_RTSC
#define USE_GET_TIME 0
/*
 * Available only on x86 architecture, thus not portable
 * moreover, when the CPU id changes the two clocks aren't
 * necessarily synchronised anymore but also the CPU switch
 * is a matter of stochastics, and it is fine but not monotonic.
 *
 * WARNING
 *
 * When using uchaos.c to seed /dev/random at boot time, keep in consideration
 * that TSC will be ready after one second while the system could have entered
 * in /init esecution at 0.1s which suggest that TSC isn't suitable, anyway.
 */
#include <x86intrin.h>
static inline uint64_t get_rdtsc_clock(uint32_t *pcpuid) {
    _mm_lfence(); return __rdtscp(pcpuid);
}
#else
#define USE_GET_TIME 1
#endif

#define USE_PRIMES_2564 0
#if     USE_PRIMES_2564
/*
 * This sequence of primes has a peculiar structure: x, y where x + y = 64.
 * Both members of each pair is a prime number, and by rotl64 are like x, -x.
 * They are only five pair of these numbers summing up 64, thus 10 = 3.32 bits.
 */
static const uint8_t primes64[20] = {  3, 61,  5, 59, 11, 53, 17, 47, 23, 41,
                                      19, 45, 29, 35, 31, 33, 13, 51,  7, 57 };

static inline uint32_t getprmx10(uint32_t x) {
    return primes64[x - ((x * 0xcccccccdULL) >> 35) * 10];
}

static inline uint32_t getprmx16(uint32_t x) {
    return primes64[ 2 + (x & 0x0f) ];
}

static inline uint32_t getprmx20(uint32_t x) {
    x -= ((x * 0xcccccccdULL) >> 35) * 10
    x +=  (x & 0x10) ? 10 : 0;
    return primes64[x];
}
#else
#define getprmx16(w) (5 + (((w) & 0x1f) << 1))
#endif

static inline uint64_t rotl64(uint64_t n, uint8_t c) {
    c &= 63; return (n << c) | (n >> ((-c) & 63));
}

#define BLOCK_SIZE 512

#ifdef _USE_LINUX_RANDOM_H
#include <linux/random.h>
#else
#define RNDADDENTROPY 0x40085203
struct rand_pool_info_buf {
    int entropy_count;
    int buf_size;
    uint32_t buf[BLOCK_SIZE >> 2];
} __attribute__((packed));
#endif

/* *** HASHING  ************************************************************* */
/*
 * The gcc compiler is good at using registers, but other simpler compilers for
 * micro architectures may be much less. So, when register as keyword is used,
 * it is impose a pragma, and if compiler cannot satisfy it, warning at least.
 */

static inline uint64_t getnstime(uint32_t *pcpuid) {
#if ! USE_GET_TIME
    if(pcpuid) return get_rdtsc_clock(pcpuid);
#endif
    struct timespec ts;                  // using sched_yield() to creates chaos,
    clock_gettime(CLOCK_MONOTONIC, &ts); // getting ns in a hot loop is the limit
    return 0x7FFFFFFF & ts.tv_nsec;      // and we want to see this limit, in VMs
}

#if 0
/*
 * This function isn't 32bit fast but a variant of the original Park-Miller
 * which has a 2^31-2 period. It is useful to introduce in the uchaos output
 * a grid-biased distribution which the murmur3 cannot remove but amplify.
 *
 * ref. Marsaglia's theorem
 */

static inline uint64_t parkmiller32(uint64_t z) {
    return (((z << 1) + 1) * 48271) % 0x7FFFFFFF;
}

/*
 * These functions below are, like the above, relatively slow compared with
 * bit operations because multiplications which impacts platforms without FPU
 * or accelerated instructions. Therefore, they can be used at the end of the
 * hot-loop producing the hash but inside they can kill the performances.
 *
 * For this reason, the uchaos multiply the 5-bit "entropy" from scheduler
 * jittering using a Xoshiro approach that mix internal states with shift
 * and rotations which are extremely fast on every architecture. Finally
 * the hash is whitened with a diffusing avalanche 32 multiplication which
 * impacts the 32+1 LSB which are mixed with 33 MLB, thus 1-bit overposition.
 *
 * ref. Lorenz strange attractors' theory
 */

static inline uint64_t murmur3(uint64_t hs) {
    register uint64_t z = hs;
    z = (z ^ (z >> 33)) * 0xff51afd7ed558ccdULL;
    z = (z ^ (z >> 33)) * 0xc4ceb9fe1a85ec53ULL;
    z = (z ^ (z >> 33));
    return z;
}

static inline uint16_t mm3ns16(uint16_t ns, uint16_t p) {
    register uint32_t z = ns;
    z = (p ^ (z >> 7)) * 0x45d9f3b;
    z = (z ^ (z >> 8)) * 0x45d9f3b;
    z = (z ^ (z >> 9));
    return (uint16_t)z;
}
#endif

#include <sched.h>

#if 0   /* RAF: unification by new rotations approach *********************** */
#define STBX 0
#define STBRSTR "stochastics branches"
#define FINAL_AVALANCHE_MLT 0xc4ceb9fe1a85ec53ULL
#define perrwrn() perr("\nWARNING: "APPNAME" isn't compiled with "STBRSTR"\n\n")
#define mm3ns32(o,h) rotl64((h * FINAL_AVALANCHE_MLT) ^ (h >> 33), 13 + ((o & 0x1f) << 1))
#define entropy(sz) ((sz << 3) - sz) // eq. to 8x (8-1)
#define minmix8
#define knuthmx
#else /* ******************************************************************** */
#define STBX 1
#define perrwrn()
#define FINAL_AVALANCHE_MLT 0xFF51AFD7ED558CCD
#define entropy(sz) ((sz << 2) - sz) // eq. to 3x (4-1)
static inline uint8_t  minmix8(uint8_t b) {
    b *= (b & 1) ? 0x4d : 0x65;
    b ^= ((b >> 3) | (b << 5));
    return b;
}
static inline uint64_t knuthmx(uint64_t iw) {
    register uint64_t w = iw;
    w  = rotl64(w, getprmx16(w));
    w *= (w & 1) ? 0x9E3779B9 : 0x45d9f3b;
    w ^= rotl64(w, (w & 2) ? 47 : 17);
    return w;
}
static inline uint64_t mm3ns32(uint64_t ks, uint64_t p) {
    register uint64_t z = ks;
    z = (p ^ (z >> 29)) * 0xff51afd7ed558ccdULL;
    z = (z ^ (z >> 31)) * 0xc4ceb9fe1a85ec53ULL;
    z = (z ^ (z >> 33)) ^ (p << 33);
    return z;
}
#endif /* ******************************************************************* */

#define PRMX USE_PRIMES_2564
#define pidx64(p) (uint64_t)pidx(p)
#define pidx(p) ((uint32_t)(uintptr_t)(p))
#define perr_app_info(a) { perr("%s %s%s%s%s%s", APPNAME, VERSION, STBX ? " w/sb"\
      : "", PRMX ? "" : " !/pr", USE_GET_TIME ? "" : " rtcs", (a) ? "\n" : ""); }
#define DJB2UPDT { tncl += ncl; tdmx = MAX(dmx, tdmx); tdmn = MIN(dmn, tdmn); }
#define DJB2RSET { DJB2UPDT; dmn = E9, dmx = 0, ncl = 0; }
#define PMDLY2NS ( ( ( dmn * pmdly ) + 127 ) >> 8 )
#define DJB2VGET ( (uint64_t)-1 )

static inline int nsleep(uint32_t ns) {
    struct timespec remaining, request = { 0, ns };
    int ret = nanosleep(&request, &remaining);
    return ret;
}

static uint64_t djb2tum(uint64_t seed, uint8_t maxn, uint32_t nsdly,
    uint32_t pmdly, uint8_t nbtls, uint8_t rset)
{
    // This is the internal state of the engine
    static uint64_t  ncl = 0,  dmn = E9,  dmx = 0;
    static uint64_t tncl = 0, tdmn = E9, tdmx = 0;
    static uint64_t nexp = 0, evnt = 0,   avg = 0;
//  static uint64_t  jmn = E9, jmx = 0,  javg = 0;

    if( seed == DJB2VGET && (ncl || tncl) ) {
        DJB2UPDT
        double mean = (double)avg / tncl;
        perr("\nLatency: %zu <%.4lg> %.4lgK ns, %.4lgK w/ ev:%zu, ex:%5.2lf%%\n",
            tdmn, mean, (double)tdmx/E3, (double)tncl/E3, evnt,
            ((double)100 * nexp)/(tncl + nexp));
        perr(  "Ratios : %.4lg <avg=1U> %.4lg, min=1U <%.4lg> %.4lg\n",
            (double)tdmn/mean, (double)tdmx/mean, mean/tdmn, (double)tdmx/tdmn);
    }

    if( rset ) DJB2RSET;

    if( seed == DJB2VGET ) return PMDLY2NS;

    if( !maxn ) return 0;

    // 0. hashing loop preparation /////////////////////////////////////////////
    /*
     * One of the most popular and efficient hash functions for strings in C is
     * the djb2 algorithm created by Dan Bernstein. It strikes a great balance
     * between speed and low collision rates. Great for text.
     *
     * 5381              Prime number choosen by Dan Bernstein, as 1010100000101
     *                   empirically is one of the best for English words text.
     * Alternatives:
     *
     * 16777619               The FNV-1 offset basis (32-bit).
     * 14695981039346656037	  The FNV-1 offset basis (64-bit).
     */
#define HSHSEED 5381
    static   uint64_t ohs = HSHSEED;
    register uint64_t hsh = ohs;
#if USE_GET_TIME
#else
    static uint32_t cpuid, oid = -1;
#endif
    uint64_t ts_tv_nsec, dff, dlt, ons = 0, ent = 0;
    uint8_t excp = 0;                // excp++ as uint8_t grants for convergence

    if( seed ) hsh ^= seed;

hashotloop:
/** HASHING LOOP START  *******************************************************/

    // 1. ns latency time retrievement /////////////////////////////////////////

#if USE_GET_TIME
    ts_tv_nsec = getnstime( NULL ) >> nbtls;
#else
    ts_tv_nsec = getnstime(&cpuid) >> nbtls;
    if( cpuid != oid && oid != -1 ) {
        // Knuth, based on gold section seeded by CPU ids event idx
        hsh = mm3ns32(hsh, ((uint64_t)cpuid << 16) | oid);
        // reschedule in the following !ons branch
        ons = 0;
        nexp++;
    }
    oid = cpuid;
#endif
    if( !ons ) {
        ons = ts_tv_nsec;
        hsh = knuthmx(hsh^ons);
        goto reschedule;
    }

    // 2. latency calculation //////////////////////////////////////////////////

    dlt = ts_tv_nsec;
#if USE_GET_TIME
    dlt = (dlt < ons) ? E9 - ons + dlt : dlt - ons;
#else
    dlt =  dlt - ons;                // overflow by uint64_t is 0xff..ff + 1 = 0
#endif
    if( !dlt ) goto reschedule;

    // 3. internal state update ////////////////////////////////////////////////

    // avg calculation can be omitted
    avg += dlt; ncl++;
    // dmn calculation is mandatory for stochastics bi-forkation turns
    if( dlt < dmn ) {
        dff = dmn - dlt; dmn = dlt;
        ent ^= -dff ^ dmn;
        evnt++;
    } else
    // dmx calculation can be omited but doing ns*=0x4d anyway
    if( dlt > dmx ) {
        dff = dlt - dmn; dmx = dlt;
        ent ^= dff ^ -dmx;
        evnt++;
    } else {
        dff = dlt - dmn;
        ent ^= ~dff ^ dmx;
    }
    // dff is jittering for the exeption manager activation
    if( dff < nsdly + (pmdly ? PMDLY2NS : 1) + excp ) {
        excp += 4;                   // increasing excp and accounting for dff
        nexp++;
    } else {
        // avg calculation can be omitted
        avg += dlt; ncl++;
        // Knuth, based on gold section seeded by 1E-3 ~ 1E-4 event idx
        if(excp) hsh = mm3ns32(hsh, ons);
        excp = 0;
    }

    // 4. entropy distillation /////////////////////////////////////////////////

    ent ^= ts_tv_nsec << 13;
    ent ^= dlt        <<  7;
    ent  = knuthmx(ent^avg);

    // 5. macro-mix in djb2-style //////////////////////////////////////////////
    /*
     * (16+1) (32-1 or 32+1) (64-1)
     *   01     10      00     11
     */
    // it consumes entropy, does rotations and forgets the state
    uint8_t b0 = ent & 0x01, b1 = ent & 0x02; ent = ent >> 2;
    hsh = ( hsh << (4 + (b0 ? b1 : 1)) ) + (b1 ? -hsh : hsh);

    // 6. entropy injection in hsh /////////////////////////////////////////////

    // it consumes entropy, and does hash the another rotation
    hsh = rotl64(hsh ^ ((ent >> 5) << 3), getprmx16(ent));

    // 7. exceptions management ////////////////////////////////////////////////

    // copying with the VMs scheduler timings: continue made by an ASM jump
    if( excp ) goto reschedule;

    // 8. preparation for the next round ///////////////////////////////////////

    if( --maxn ) {
        ons = ts_tv_nsec;
reschedule:
        sched_yield();
        goto hashotloop;             // a loop made by two ASM jumps
    }

/** HASHING LOOP CLOSE  *******************************************************/

    // 9. finalising w/ a 32+1 bit mix /////////////////////////////////////////

    ohs = mm3ns32(hsh, ohs);

    return ohs;
}

static inline uint8_t trndbyte() {
    sched_yield(); return 0xFF & getnstime(NULL);
}

static inline uint8_t *bin2str(uint8_t *buf, uint32_t nmax) {
    for(register uint32_t i = 0; i < nmax; i++)
        if(!buf[i]) buf[i--] = trndbyte();
    return buf;
}

uint64_t *str2ht64(const uint8_t *str, uint32_t *size, uint32_t nsdly,
    uint32_t pmdly, uint8_t nbtls, uint8_t rset)
{
    if (!str || !size) return NULL;

    // 0. Calculate the string lenght
    uint32_t len = *size;
    if(!len) {
        len = strlen((char *)str);
        if (len == 0) return NULL;
    }

    // 1. Calculate padding and allocation
    // We need enough 64-bit blocks to cover n bytes.
    uint32_t nwords = (len + 7) >> 3;
    uint32_t nbytes = nwords << 3;

    // 2. Allocate aligned memory for the processed string
    uint8_t *str64 = NULL;
    if(posix_memalign((void **)&str64, 64, nbytes) || !str64) {
        perror("posix_memalign");
        return NULL;
    }

    // 3. Determine rotation offset using monotonic time
    uint32_t k = trndbyte() % len;
    // 4. Perform rotation into the new buffer
    // Copy from k to end
    memcpy(str64, str + k, len - k);
    // Copy from beginning to k
    //perr("k = %d, len = %ld \n", k, len);
    if (k) memcpy(str64 + (len - k), str, k);
    // Padding with zeros
    memset(&str64[len], 0, nbytes - len);

    // 5. Generate the uint64_t array
    // We allocate a separate array for hashes if that was the intent, or we cast
    // the rotated string. Based on your code, you want a hash per 8-byte block.
    uint64_t *h = NULL;
    if(posix_memalign((void **)&h, 64, nwords << 3) || !h) {
        perror("posix_memalign");
        free(str64);
        return NULL;
    }
    *size = nwords;
    //perr("len: %ld, wd: %ld\n", len, nwords);

    // 6. Producing the hashing sequence
    uint64_t *p = (uint64_t *)str64;
    for (uint64_t i = 0, n = 8; i < nwords; i++, n += 8) {
        // Processing each 8-byte chunk of the rotated/padded string
        h[i] = djb2tum(p[i], 1 + !!rset, nsdly, pmdly, nbtls, 0);
        if ( rset && n >= ((uint64_t)1 << rset) ) {
            n = 0; djb2tum(0, 0, 0, 0, 0, 1);
        }
    }

    // 7. free the string memory and return the hash
    free(str64);
    return h;
}

/** I/O ***********************************************************************/

static inline uint32_t writebuf(int fd, const uint8_t *buffer, uint32_t ntwr) {
    uint32_t tot = 0;
    while (ntwr > tot) {
        errno = 0;
        int nw = write(fd, buffer + tot, ntwr - tot);
        if (nw < 1) {
            if (errno == EINTR) continue;
            perror("write");
            exit(EXIT_FAILURE);
        }
        tot += nw;
   }
   return tot;
}

static inline uint32_t readbuf(int fd, uint8_t *buffer, uint32_t size, bool intr) {
    uint32_t tot = 0;
    while (size > tot) {
        errno = 0;
        int nr = read(fd, buffer + tot, size - tot);
        if (nr == 0) break;
        if (nr < 0) {
            if (errno == EINTR) {
              if(intr) return tot;
              else continue;
            }
            perror("read");
            exit(EXIT_FAILURE);
        }
        tot += nr;
    }
    return tot;
}

static inline uint32_t readblocks(int fd, uint8_t *buf, uint32_t nblks) {
    uint8_t inp[BLOCK_SIZE], fst[BLOCK_SIZE];
    // Reading max 8 blocks to limit the overflow at min 5 LSB bits,
    // considering that ASCII text is almost all chars in 32-122 range.
    // Anyway, pre-processing the input may help but it shouldn't matter
    // when the final aim is to feed uchaos for providing randomness.
    uint32_t n, maxn = 0;
    memset(buf, 0, BLOCK_SIZE);
    // Input size 16 * 512 = 8K as relevant initial dmesg log before init
    for(int i = 0; i < nblks; i++) {
        n = readbuf(fd, inp, BLOCK_SIZE, 0);
        maxn = MAX(maxn, n);

        if(i) {
            if(n < BLOCK_SIZE) {
                memcpy(&inp[n], fst, BLOCK_SIZE-n);
                n = BLOCK_SIZE;
            }
        } else {
            if(n == BLOCK_SIZE)
                memcpy(fst, inp, BLOCK_SIZE);
        }
        // mixing the input by 64-bit words
        n = (n + 7) >> 3;
        uint64_t *ip = (uint64_t *)inp, *bp = (uint64_t *)buf;
        for (uint32_t a = 0; a < n; a++)
            bp[a] =  ip[a] ^ rotl64(bp[a], a);
    }
    return maxn;
}

/* ** main & its supporters ************************************************* */

// Funzione per ottenere il tempo in nanosecondi
uint64_t get_nanos() {
    static uint64_t start = 0;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (!start) {
        start = (uint64_t)ts.tv_sec * E9 + ts.tv_nsec;
        return start;
    }
    return ((uint64_t)ts.tv_sec * E9 + ts.tv_nsec) - start;
}

static inline void usage(const char *name, const char *cmdn, const uint8_t qlvl) {
    perr("\n"\
"%s "VERSION" reads from stdin, stats on stderr, and rand on stdout.\n"\
"|\n"\
"\\_ Usage: %s [-h,q%s,V] [-T/K/M/G N] [-d,p,s,r N] [-k /dev/rnd]\n"\
" |\n"\
" |    -qq,-q: (extra) quiet run for scripts automation\n"\
" |    -K/-M/-G: kilo/mega/giga bytes of data w/ stats on\n"\
" |    -S: low-entropy VMs seeding settings (r31,d3,i16,K4)\n"\
" |    -Z: the same as -S but with a reset at 2^19 bytes\n"\
" |    -T: number of collision tests x2 on the same input\n"\
" |    -d: number of ns above min as the minimum delay\n"\
" |    -p: number of parts as min/256 ns above the min\n"\
" |    -s: number of bits to left shift on ns timings\n"\
" |    -r: number of preliminary runs (default: 1)\n"\
" |    -k: randomness injection in kernel by ioctl\n"\
" |    -i: number of 512B-blocks to read from stdin\n"\
" |    -h/-v: shows this help / appname and version\n"\
" |\n "\
"\\_ With -pN is suggested -r31 or -r63 for stats pre-evaluation.\n"\
" |\n"\
" \\_ Typeical: [sudo] %s -Sk /dev/random (at /init time)\n\n",
        name, cmdn, (qlvl > 1) ? "^" : "", cmdn);
    return;
}

#define APPNAME "uChaos"
#define STCX STOCHASTIC_BRANCHES
#define perrprms(s,p) perr("%s s:%d, q:%d, p:%d%%:%dns, d:%d, r:%d, i:%d, Z:%d\n\n",\
                      s, nbtls, quiet, pmdly, p, nsdly, nrdry, nblks, rset)

int main(int argc, char *argv[]) {
    struct rand_pool_info_buf entrnd;
    uint8_t *str = NULL, nbtls = 0, prsts = 0, quiet = 0, nblks = 1, rset = 0;
    uint32_t ntsts = 1, nsdly = 0;
    uint32_t nrdry = 1, pmdly = 0;
    int devfd = 0;

    // Collect arguments from optional command line parameters
    while (1) {
        int opt = getopt(argc, argv, "hvSZG:M:K:T:s:d:p:r:k:i:q");
        if(opt == 'S' || opt == 'Z') {
            nsdly = 3; nblks = 16; nrdry = 31; ntsts = 8;
            rset = (opt == 'Z') ? 19 : 0;
            perrwrn();
        } else
        if(opt == 'v') {
            perr_app_info(2);
            return 0;
        } else
        if(opt == 'q') {
            quiet = (++quiet) ? quiet : 2;
        } else
        if(opt == '?' || opt == 'h') {
            char *p, *q = argv[0];
            if(q) for(p = q; *p; p++) if(*p == '/') q = p+1;
            usage(APPNAME, q ? q : "uchaos", quiet);
            return 0;
        } else
        if(opt == -1) {
            break;
        }

        if(!optarg) continue;

        long x = atoi(optarg);
        switch (opt) {
            // ABS sanitises the parametric inputs
            case 's': nbtls = ABS(x); break;
            case 'd': nsdly = ABS(x); break;
            case 'r': nrdry = ABS(x); break;
            case 'p': pmdly = ABS(x); break;
            case 'i': nblks = ABS(x); break;
            case 'k': devfd = open(optarg,O_WRONLY); break;
            case 'G': ntsts = ABS(x); ntsts <<= 21 ; prsts = 1; break;
            case 'M': ntsts = ABS(x); ntsts <<= 11 ; prsts = 1; break;
            case 'K': ntsts = ABS(x); ntsts <<=  1 ; prsts = 1; break;
            case 'T': ntsts = ABS(x); ntsts <<=  1 ; prsts = 1; break;
        }
    }

    if (devfd < 0) {
        perror("open device");
        return EXIT_FAILURE;
    }
    if(quiet) prsts = 0;

    // Counting time of running starts here, after parameters
    (void) get_nanos();

    if (posix_memalign((void **)&str, 64, BLOCK_SIZE + 8) || !str) {
        perror("posix_memalign");
        return EXIT_FAILURE;
    }

    uint32_t n = (nblks < 2) ? readbuf(STDIN_FILENO, str, BLOCK_SIZE, 0) \
                             : readblocks(STDIN_FILENO, str, nblks);
    if(n < 1) return EXIT_FAILURE;     // djb2tum(9 code refactored thus not anymore
    //if (nblks > 1) bin2str(str, n); // necessary because djb2tum() born for text,
    str[n] = 0;                      // refactoring it for binary input, is the way.

    for(uint32_t a = nrdry; a; a--) {
        uint32_t size = n;
        uint64_t *hsh = str2ht64(str, &size, nsdly, pmdly, nbtls, 0);
        if(!hsh) { return EXIT_FAILURE; }
        else { free(hsh); } hsh = NULL;
    }

    double avgbc = 0, avgmx = 0, avgmn = 256;
    uint64_t bic = 0, max = 0, min = 256, avg = 0;
    uint64_t nk = 0, nt = 0, nx = 0, nn = 0, mt = 0;

    if(quiet < 2) {
        perr_app_info(0);
        if(prsts) { // RAF, TODO: dealing with size one.
            perr("; repetitions: ");
            if(nblks < 2) {
                perr("too short input, try longer!\n\n");
                prsts = 0;
            }
        } else perrprms("", -1);
    }

    for (uint32_t a = ntsts; a; a--) {
        // hashing
        uint32_t size = n;
        uint64_t stns = get_nanos();
        uint64_t *hsh = str2ht64(str, &size, nsdly, pmdly, nbtls, rset);
        if(!hsh) return EXIT_FAILURE;
        mt += get_nanos() - stns;

        uint32_t sz = size << 3;
        if(devfd) {
            entrnd.buf_size = sz;
            entrnd.entropy_count = entropy(sz);
            memcpy((uint8_t *)entrnd.buf, (uint8_t *)hsh, sz);
            #define OPTNK "option -k is designed for /dev/[u]random only"
            if (ioctl(devfd, RNDADDENTROPY, &entrnd) < 0 && errno != EINTR) {
                if(errno == ENOTTY) {
                  perr("\nERROR: "APPNAME" "OPTNK"\n\n");
                } else perror("ioctl entrnd");
                return EXIT_FAILURE;
            }
            if (quiet < 2) // avoid the need of >/dev/null
                writebuf(STDOUT_FILENO, (uint8_t *)hsh, sz);
        } else {
                writebuf(STDOUT_FILENO, (uint8_t *)hsh, sz);
        }

        // single run
        if(ntsts < 2) return 0;

        // skip stats
        if(!prsts) continue;

        // self-evaluation of the output including the check for repetitions
        avg = 0, nn = 0;
        for (uint32_t n = 0; n < size; n++) {
            for (uint32_t i = n + 1; i < size; i++) {
                if (hsh[i] == hsh[n]) {
                    if(prsts) perr("%d:%d ", n, i);
                    nk++; continue;
                }
                uint64_t cb = hsh[i] ^ hsh[n];
                int ham = 0;
                for (int a = 0; a < 64; a++)
                    ham += (cb >> a) & 0x01;
                bic += ham;
                avg += ham;
                if(max < ham) max = ham;
                if(min > ham) min = ham;
                nx++;
                nn++;
            }
        }

        double curavg = (double)avg / nn;
        if(avgmx < curavg) avgmx = curavg;
        if(avgmn > curavg) avgmn = curavg;
        avgbc += curavg;
        nt += size;

        free(hsh); hsh = NULL;
        sched_yield();  // Statistics are a block of CPU data-crunching but also
                        // a predictable delay which sched_yield() can jeopardise.
                        // Stats makes the large size output slower 1.7x than -q.
    }

    if(!prsts) return 0;

    uint64_t rt = get_nanos();
    perr("%s\n", nk ? ", status KO" : "0, status OK");
    perr("\n");

    perr("Tests: %u w/ duplicates %zu over ", ntsts, nk);
    if((nt >> 3) > E6)
        perr("%.2lfM hashes (%.4lg ppm)\n", (double)nt/E6, (double)E6*nk/nt);
    else
        perr("%.1lfK hashes (%.4lg ppm)\n", (double)nt/E3, (double)E6*nk/nt);

    avgbc /= ntsts;
    double bic_nx_absl = (double)bic / nx;
    double bic_nx = (double)100 / 64 * bic_nx_absl;
    #define devppm(v,a) ( ((double)v-a) * E6 / a )

    perr("Hamming <weight>: %.4lf%% ~ 50%% by (%+.4lg ppm)\n",
        bic_nx, devppm(bic_nx, 50));
    perr("Hamming distance: %.0lf <%.6lf> %.0lf over %.4lgK XORs\n",
        (double)min, bic_nx_absl, (double)max, (double)nx/E3);
    perr("Hamming dist/avg: %.4lf < 1U:32 %+.4lg ppm > %.4lf\n",
        avgmn/bic_nx_absl, devppm(bic_nx_absl, 32), avgmx/bic_nx_absl);

    perr("\n");
    perr("Perform: exec %.3lgs, %.3lg MB/s; hash %.3lgs, %.4lg KH/s",
        (double)rt/E9, (double)E6*nt/(rt<<7), (double)mt/E9, (double)E6*nt/mt);

    uint32_t pmns = (uint32_t)djb2tum(DJB2VGET, 0, 0, pmdly, 0, 0);
    perrprms("Setting:", pmns);

    return 0; // exit() do free()
}

