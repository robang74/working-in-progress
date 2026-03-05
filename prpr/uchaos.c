/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 license
 */
#define VERSION "v0.2.9.6"
/* Quick 2k test: cat uchaos.c  | ./chaos -T 2048 | ent
 * Boot log test: cat dmesg.txt | ./uchaos -S -M2 | ent
 *
 * Compile w/libc:      gcc uchaos.c -O3 --fast-math -Wall -o uchaos -s
 * Compile w/musl: musl-gcc uchaos.c -O3 --fast-math -Wall -o uchaos -s -static
 * Compile option: -D_USE_GET_RTSC, -D_USE_STOCHASTIC_BRANCHES, -D_USE_LINUX_RANDOM_H
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
#if USE_GET_TIME
    struct timespec ts;                         // using sched_yield() to creates chaos,
    clock_gettime(CLOCK_MONOTONIC, &ts);        // getting ns in a hot loop is the limit
    return 0x7FFFFFFF & ts.tv_nsec;             // and we want to see this limit, in VMs
#else
    return get_rdtsc_clock(pcpuid);
#endif
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

#define pmdly2ns ( ( ( (uint64_t)dmn * pmdly ) + 127 ) >> 8 )

#define STBRSTR "stochastics branches"

#define USE_STOCHASTIC_BRANCHES   // RAF: unification by new rotations approach
#ifdef  USE_STOCHASTIC_BRANCHES

  #define STBX 1
  #define perrwrn()
  #define FINAL_AVALANCHE_MLT 0xFF51AFD7ED558CCD
  #define entropy(sz) ((sz << 2) - sz) // eq. to 3x (4-1)
  static inline uint8_t  minmix8(uint8_t b) {
      b *= (b & 1) ? 0x4d : 0x65;
      b ^= ((b >> 3) | (b << 5));
      return b;
  }
  static inline uint64_t knuthmx(uint64_t w) {
  #if USE_PRIMES_2564
      w  = rotl64(w, getprmx16(w));
  #else
      w  = rotl64(w, 5 + ((w & 0x1f) << 1));
  #endif
      w *= (w & 1) ? 0x9E3779B9 : 0x45d9f3b;
      return w ^ ( (w >> 17) | (w << 47) ) ;
  }
  static inline uint64_t mm3ns32(uint64_t ks, uint64_t p) {
      register uint64_t z = ks;
      z = (p ^ (z >> 29)) * 0xff51afd7ed558ccdULL;
      z = (z ^ (z >> 31)) * 0xc4ceb9fe1a85ec53ULL;
      z = (z ^ (z >> 33)) ^ p << 33;
      return z;
  }

#else

  #define STBX 0
  #define FINAL_AVALANCHE_MLT 0xc4ceb9fe1a85ec53ULL
  #define perrwrn() perr("\nWARNING: "APPNAME" isn't compiled with "STBRSTR"\n\n")
  #define mm3ns32(o,h) rotl64((h * FINAL_AVALANCHE_MLT) ^ (h >> 33), 13 + ((o & 0x1f) << 1))
  #define entropy(sz) ((sz << 3) - sz) // eq. to 8x (8-1)
  #define minmix8
  #define knuthmx

#endif

#define PRMX USE_PRIMES_2564
#define STOCHASTIC_BRANCHES STBX
#define perr_app_info(a) { perr("%s %s%s%s%s%s", APPNAME, VERSION,\
    STBX ? " w/sb" : "", PRMX ? "" : " !/pr", USE_GET_TIME?" rtcs":"", (a) ? "\n" : ""); }

#define GETVAL (const uint8_t *)(-1)

static uint64_t djb2tum(const uint8_t *str, uint8_t maxn, uint64_t seed,
    const uint32_t nsdly, const uint32_t pmdly, const uint8_t nbtls)
{
    static uint64_t ncl = 0, dmn = E9, nexp = 0, evnt = 0, avg = 0, dmx = 0;

    if( pmdly && str == GETVAL ) return pmdly2ns;
    if( !str ) {
        if( ncl ) {
            double mean = (double)avg / ncl;
            perr("\nLatency: %.0lf <%.1lf> %.0lf ns over %.0lf K (w/ %.0lf + %.0lf)\n",
                (double)dmn, mean, (double)dmx, (double)ncl/E3, (double)evnt, (double)nexp);
            perr(  "Ratios : on avg %.2lf <1U> %.2lf, on min 1U <%.2lf> %.2lf\n",
                (double)dmn/mean, (double)dmx/mean, mean/dmn, (double)dmx/dmn);
        }
        if( pmdly && !seed && !maxn && !nsdly && !nbtls )
            return pmdly2ns;
        return 0;
    }
    if( !*str || !maxn ) return 0;

    // 0. hot loop preparation /////////////////////////////////////////////////
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
    register uint64_t hsh = HSHSEED;
    if( seed )        hsh = seed;

#if USE_GET_TIME
#else
    static uint32_t cpuid, oid = -1;
#endif
    uint64_t ts_tv_nsec, dff, dlt = 0, ons = 0, ent = 0, chr = *str;
    uint8_t excp = 0;

hashotloop:                          // a loop made by ASM jumps

    // 1. ns latency time retriviement /////////////////////////////////////////
#if USE_GET_TIME
    ts_tv_nsec = getnstime( NULL ) >> nbtls;
#else
    ts_tv_nsec = getnstime(&cpuid) >> nbtls;
    if( cpuid != oid ) {
          oid  = cpuid;
          ons  = 0;                  // reschedule in the following !ons branch
    }
    oid = cpuid;
#endif
    if( !ons ) {
        ons  = ts_tv_nsec;
        // RAF, TODO: inject entropy here
        goto reschedule;
    }

    // 2. jitter calculation ///////////////////////////////////////////////////

    dlt = ts_tv_nsec;
#if USE_GET_TIME
    dlt = (dlt < ons) ? E9 - ons + dlt : dlt - ons;
#else
    dlt =  dlt - ons;                // overflow by uint64_t is 0xff..ff + 1 = 0
#endif
    ons = ts_tv_nsec;

    // 3. internal state update ////////////////////////////////////////////////

    excp = 0;
    // avg calculation can be omitted
    avg += dlt; ncl++;
    // dmn calculation is mandatory for stochastics bi-forkation turns
    if( dlt < dmn ) {
        dff = dmn - dlt; dmn = dlt; dlt = dff;
        ent = ent - minmix8(dlt); evnt++;
    } else
    // dmx calculation can be omited but doing ns*=0x4d anyway
    if( dlt > dmx ) {
        dmx = dlt;
        ent = ent + minmix8(dlt); evnt++;
    }
    // for the exeption manager activation
    if( dlt < nsdly + (pmdly ? pmdly2ns : 1) )
        excp = 1;

    // 4. entropy distillation /////////////////////////////////////////////////

    ent ^= (ent << 6) ^ ts_tv_nsec;
    ent ^= (ent << 3) ^ dlt;
    ent  = knuthmx(ent);

    // 5. macro-mix in djb2-style //////////////////////////////////////////////
    /*
     * (16+1) (32-1 or 32+1) (64-1)
     *   01     10      00     11
     */
    // consumed entropy, do rotations and forgot the state
    uint8_t b0 = ent & 0x01, b1  = ent & 0x02; ent = ent >> 2;
    hsh = ( hsh << (4 + (b0 ? b1 : 1)) ) + (b1 ? -hsh : hsh);

    // 6. entropy injection in hsh /////////////////////////////////////////////

#if USE_PRIMES_2564
    hsh ^= (ent << 2) ^ rotl64(chr,   getprmx16(ent));
#else
    hsh ^= (ent << 2) ^ rotl64(chr, 3 + (0x1f & ent));
#endif

    // 7. exceptions management ////////////////////////////////////////////////
    #define pidx(p) ((uint32_t)(uintptr_t)(p))
    if( excp || hsh == ohs ) {       // copying with the VMs scheduler timings
reschedule:
        str--;                       // apply changes but repeat the action
        nexp++;
        sched_yield();
        // Knuth, based on gold section seeded by 1E-3 ~ 1E-4 event idx
        hsh = mm3ns32(hsh, pidx(&chr));
        goto hashotloop;             // continue made by an ASM jump
    }

    // 8. preparation for the next round ///////////////////////////////////////

    if( (chr = *++str) && maxn-- ) {
        ohs = hsh;
        sched_yield();
        goto hashotloop;             // a loop made by two ASM jumps
    }

    // 9. finalising w/ a 32+1 bit mix /////////////////////////////////////////

    dff = hsh;
    hsh = mm3ns32(hsh, ohs);
    ohs = dff;

    return hsh;
}

#define USE_EXP_COMPR 0 // RAF: since v0.2.9.3 this mode is not yet useful w/0°K
                        //      qemu VMs and good randomnes is produced also w/o

uint64_t *str2ht64(uint8_t *str, uint64_t **ph,  uint32_t *size,
    const uint32_t nsdly, const uint32_t pmdly, const uint8_t nbtls)
{
    if (!str || !size) return NULL;

    uint32_t n = strlen((char *)str);
    if (n == 0) return NULL;

    // 1. Determine rotation offset using monotonic time
    uint32_t k = getnstime(NULL) % n;

    // 2. Calculate padding and allocation
    // We need enough 64-bit blocks to cover n bytes.
    uint64_t *h = NULL;
    uint32_t num_blocks = (n + 7) >> 3;
    uint32_t total_bytes = num_blocks << 3;
    if(ph) {
        if(*size > 0 && num_blocks != *size) {
            perror("*size != expected");
            return NULL;
        }
        h = *ph;
    }

    // Allocate aligned memory for the processed string
    uint8_t *rotated_str = NULL;
    if (posix_memalign((void **)&rotated_str, 64, total_bytes)) {
        perror("posix_memalign");
        return NULL;
    }

    // 3. Perform rotation into the new buffer
    // Copy from k to end
    memcpy(rotated_str, str + k, n - k);
    // Copy from beginning to k
    if (k > 0) {
        memcpy(rotated_str + (n - k), str, k);
    }
    // Padding with zeros
    for(uint32_t i = n; i < total_bytes; i++)
        rotated_str[i] = 0;

    // 4. Generate the uint64_t array
    // We allocate a separate array for hashes if that was the intent, or we cast
    // the rotated string. Based on your code, you want a hash per 8-byte block.
    if(!h) {
        if(posix_memalign((void **)&h, 64, num_blocks << 3)) {
            perror("posix_memalign");
            free(rotated_str);
            return NULL;
        }
    }
    for (uint32_t i = 0; i < num_blocks; i++) {
        // Process each 8-byte chunk of the rotated/padded string
        h[i] = djb2tum(rotated_str + (i << 3), 8, 0, nsdly, pmdly, nbtls);
    }
    free(rotated_str);

    *size = num_blocks;
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
    uint32_t maxn = 0;
    memset(buf, 0, BLOCK_SIZE);
    // Input size 16 * 512 = 8K as relevant initial dmesg log before init
    for(int i = 0; i < nblks; i++) {
        uint32_t n = readbuf(fd, inp, BLOCK_SIZE, 0);

        if(i) {
            if(n < BLOCK_SIZE) {
                memcpy(&inp[n], fst, BLOCK_SIZE-n);
                n = BLOCK_SIZE;
            }
        } else {
            if(n == BLOCK_SIZE)
                memcpy(fst, inp, BLOCK_SIZE);
        }
        maxn = MAX(maxn, n);

        for (uint32_t a = 0; a < n; a++) {
            buf[a] ^= inp[a];
            buf[a] ^= (buf[a] << 3) | (buf[a] >> 5);
        }
    }
    return maxn;
}

static inline uint8_t *bin2str(uint8_t *buf, uint32_t nmax) {
    for(register uint32_t i = 0; i < nmax; i++) {
        if(!buf[i]) {
            buf[i--] = 0xFF & getnstime(NULL);
            sched_yield();
        }
    }
    return buf;
}

/* ** main & its supporters ************************************************* */

// Funzione per ottenere il tempo in nanosecondi
uint64_t get_nanos() {
    static uint64_t start = 0;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (!start) {
        start = (uint64_t)ts.tv_sec * 1000000000L + ts.tv_nsec;
        return start;
    }
    return ((uint64_t)ts.tv_sec * 1000000000L + ts.tv_nsec) - start;
}

static inline void usage(const char *name, const char *cmdn, const uint8_t qlvl) {
    perr("\n"\
"%s "VERSION" reads from stdin, stats on stderr, and rand on stdout.\n"\
"|\n"\
"\\_ Usage: %s [-h,q%s,V] [-T/K/M/G N] [-d,p,s,r N] [-k /dev/rnd]\n"\
" |\n"\
" |    -q: quiet run for the scripts automation (-qqk)\n"\
" |    -K/-M/-G: kilo/mega/giga bytes of data w/ stats on\n"\
" |    -S: low-entropy VMs seeding settings (r31,d3,I16,qqK4)\n"\
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
#define perrprms(s,p) perr("%s: s(%d), q(%d), p(%d:%d), d(%d), r(%d), i(%d)\n\n",\
                      s, nbtls, quiet, pmdly, p, nsdly, nrdry, nblks)

int main(int argc, char *argv[]) {
    struct rand_pool_info_buf entrnd;
    uint8_t *str = NULL, nbtls = 0, prsts = 0, quiet = 0, nblks = 1;
    uint32_t ntsts = 1, nsdly = 0;
    uint32_t nrdry = 1, pmdly = 0;
    int devfd = 0;

    // Collect arguments from optional command line parameters
    while (1) {
        int opt = getopt(argc, argv, "hvSG:M:K:T:s:d:p:r:k:i:q");
        if(opt == 'S') {
            nsdly=7; nblks=16; nrdry=31; ntsts=(4<<1); quiet++;
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
#if USE_EXP_COMPR
    if(quiet < 2)
        perr("\n>>> WARNING!! <<< "APPNAME" is compiled with USE_EXP_COMPR\n\n");
#endif

    // Counting time of running starts here, after parameters
    (void) get_nanos();

    if (posix_memalign((void **)&str, 64, BLOCK_SIZE + 8)) {
        perror("posix_memalign");
        return EXIT_FAILURE;
    }

    uint32_t n = (nblks < 2) ? readbuf(STDIN_FILENO, str, BLOCK_SIZE, 0) \
                             : readblocks(STDIN_FILENO, str, nblks);
    if(n < 1) return EXIT_FAILURE;
    if (nblks > 1) bin2str(str, n);   // necessary because djb2tum() born for text,
    str[n] = 0;                       // refactoring it for binary input, is the way.

    uint32_t size = 0;
    uint64_t *h = NULL;
    for(uint32_t a = 0; a < nrdry; a++)
        h = str2ht64(str, &h, &size, nsdly, pmdly, nbtls);

    double avgbc = 0, avgmx = 0, avgmn = 256;
    uint64_t bic = 0, max = 0, min = 256, avg = 0, mt = 0;
    uint64_t nk = 0, nt = 0, nx = 0, nn = 0;

    if(quiet < 2) {
        perr_app_info(0);
        if(prsts) { // RAF, TODO: dealing with size one.
            perr("; repetitions: ");
            if(size < 2) {
                perr("too short input, try longer!\n\n");
                prsts = 0;
            }
        } else perrprms("", -1);
    }

#if USE_EXP_COMPR
    register uint64_t output = 0;
#endif
    for (uint32_t a = ntsts; a; a--) {
        // hashing
        uint64_t stns = get_nanos();
        h = str2ht64(str, &h, &size, nsdly, pmdly, nbtls);
        if(!h) return EXIT_FAILURE;
        mt += get_nanos() - stns;

        uint32_t sz = size << 3;
        if(devfd) {
            entrnd.buf_size = sz;
            entrnd.entropy_count = entropy(sz);
            memcpy((uint8_t *)entrnd.buf, (uint8_t *)h, sz);
            #define OPTNK "option -k is designed for /dev/[u]random only"
            if (ioctl(devfd, RNDADDENTROPY, &entrnd) < 0 && errno != EINTR) {
                if(errno == ENOTTY) {
                  perr("\nERROR: "APPNAME" "OPTNK"\n\n");
                } else perror("ioctl entrnd");
                return EXIT_FAILURE;
            }
            if (quiet < 2) // avoid the need of >/dev/null
                writebuf(STDOUT_FILENO, (uint8_t *)h, sz);
        } else {
#if ! USE_EXP_COMPR
                writebuf(STDOUT_FILENO, (uint8_t *)h, sz);
#else
            static uint32_t ncnt = 0, nfld = 0;
            for (uint32_t i = 0; i < size; i++) {
                output ^= rotl64(h[i], i);
                if( ++ncnt >> (nfld >> 8) ) {
                    stns = mm3ns32(output, output);
                    writebuf(STDOUT_FILENO, (const uint8_t *)&stns, 8);
                    nfld++;
                }
             }
#endif
        }

        // single run
        if(ntsts < 2) return 0;

        // skip stats
        if(!prsts) continue;

        // testing
        // expected zero collisions and the nested-for can be verified by:
        // xxd -p -c 8 data.test | sort | uniq -c | awk '$1 >1' | wc -l

        avg = 0, nn = 0;
        for (uint32_t n = 0; n < size; n++) {
            for (uint32_t i = n + 1; i < size; i++) {
                if (h[i] == h[n]) {
                    if(prsts) perr("%d:%d ", n, i);
                    nk++; continue;
                }
                uint64_t cb = h[i] ^ h[n];
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

#if 0                      // Just for test
        free(h); h = NULL; // passing to str2ht64 a valid (h, size) should reused it
#endif
        sched_yield();     // Statistics are a block of CPU data-crunching but also
                           // a predictable delay which sched_yield() can jeopardise.
                           // Stats makes the large size output slower 1.7x than -q.
    }
#if USE_EXP_COMPR
    for(uint64_t o = output; output;) {
        writebuf(STDOUT_FILENO, (uint8_t *)&o, 8); break;
    }
#endif

    if(!prsts) return 0;

    uint64_t rt = get_nanos();
    perr("%s\n", nk ? ", status KO" : "0, status OK");
    perr("\n");

    perr("Tests: %d w/ duplicates %.0lf over ", ntsts, (double)nk);
    if((nt >> 3) > E6)
        perr("%.2lf M hashes (%.2lf ppm)\n", (double)nt/E6, (double)E6*nk/nt);
    else
        perr("%.1lf K hashes (%.2lf ppm)\n", (double)nt/E3, (double)E6*nk/nt);

    avgbc /= ntsts;
    double bic_nx_absl = (double)bic / nx;
    double bic_nx = (double)100 / 64 * bic_nx_absl;
    #define devppm(v,a) ( (v-a) * E6 / a )

    perr("Hamming weight, avg is %.4lf %% expected 50 %% (%+.1lf ppm)\n",
        bic_nx, devppm(bic_nx, 50));
    perr("Hamming distance: %.0lf < %.5lf > %.0lf over %.4lg K XORs\n",
        (double)min, bic_nx_absl, (double)max, (double)nx/E3);
    perr("Hamming dist/avg: %.5lf < 1U:32 %+.1lf ppm > %.5lf\n",
        avgmn/bic_nx_absl, devppm(bic_nx_absl, 32), avgmx/bic_nx_absl);

    perr("\n");
    perr("Times: running %.3lfs, %.2lf MB/s; hashing %.3lfs, %.1lf KH/s",
        (double)rt/E9, (double)E6*nt/(rt<<7), (double)mt/E9, (double)E6*nt/mt);

    uint32_t pmns = (uint32_t)djb2tum(0, 0, 0, 0, pmdly, 0);
    perrprms("Parameter settings", pmns);

    return 0; // exit() do free()
}

