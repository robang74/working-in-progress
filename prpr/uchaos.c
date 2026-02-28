/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 license
 *
 * Version: v0.2.4.2
 * Quick 2k test: cat uchaos.c  | ./chaos -T 2048 | ent
 * Boot log test: cat dmesg.txt | ./uchaos -i 16 -r64 | ent
 * Compile w/libc: gcc uchaos.c -O3 --fast-math -Wall -o uchaos [-D_USE_GET_RTSC]
 * Compile w/musl: musl-gcc uchaos.c -O3 --fast-math -Wall -static -s -o uchaos
 * Test with: ent, dieharder, PractRand RNG_test (compiled for Ubuntu 22.04 x64)
 *      drive.google.com/file/d/17ymBcxfO2pA8ET7T4ZxiiO2EYW6_F8Lu/view
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
#define E3 1000L
#define E6 1000000L
#define E9 1000000000L
#define MAX_READ_SIZE 4096
#define MAX_COMP_SIZE (MAX_READ_SIZE << 1)
#define ABS(a)    ( ( (a) < 0 )  ? -(a) : (a) )
#define MIN(a,b)  ( ( (a) < (b) ) ? (a) : (b) )
#define MAX(a,b)  ( ( (a) > (b) ) ? (a) : (b) )
#define ALGN64(n) ( ( ( (n) + 63) >> 6 ) << 6 )
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

#define USE_PRIMES_2564 1
#if     USE_PRIMES_2564
/*
 * This sequence of primes has a peculiar structure: x, y where x + y = 64.
 * Both members of each pair is a prime number, and by rotl64 are like x, -x.
 * They are only five pair of these numbers summing up 64, thus 10 = 3.32 bits.
 */
static const uint8_t primes64[10] = { 3, 61, 5, 59, 11, 53, 17, 47, 23, 41 };
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
#else
    if(pcpuid) return get_rdtsc_clock(pcpuid);
#endif
    struct timespec ts;                         // using sched_yield() to creates chaos,
    clock_gettime(CLOCK_MONOTONIC, &ts);        // getting ns in a hot loop is the limit
    return ts.tv_nsec;                          // and we want to see this limit, in VMs
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

static inline uint64_t mm3ns32(uint64_t ks, uint64_t p) {
    register uint64_t z = ks;
    z = (p ^ (z >> 31)) * 0xff51afd7ed558ccdULL;
    z = (z ^ (z >> 32)) * 0xc4ceb9fe1a85ec53ULL;
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

static uint64_t djb2tum(const uint8_t *str, uint8_t maxn, uint64_t seed,
    const uint32_t nsdly, const uint32_t pmdly, const uint8_t nbtls)
{
    static double dmx = 0;
    static uint64_t ncl = 0, dmn = -1, nexp = 0, avg = 0;

    if( !str ) {
        if( ncl ) {
            double mean = (double)avg / ncl;
            perr("\nTime deltas avg: %.0lf <%.1lf> %.0lf ns over %.0lfK (+%.0lf) values\n",
                (double)dmn, mean, dmx, (double)ncl/E3, (double)nexp);
            perr("Ratios over avg: %.2lf <1U> %.2lf, over min: 1U <%.2lf> %.2lf\n",
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
     */
    register uint64_t hsh = 5381;
    /*
     * 5381              Prime number choosen by Dan Bernstein, as 1010100000101
     *                   empirically is one of the best for English words text.
     * Alternatives:
     *
     * 16777619               The FNV-1 offset basis (32-bit).
     * 14695981039346656037	  The FNV-1 offset basis (64-bit).
     */
    if( seed ) hsh = seed;

    static uint64_t ohs = 5381;
#if USE_GET_TIME
#else
    static uint32_t cpuid, oid = -1;
#endif
    uint32_t nstw;
    uint64_t ts_tv_nsec, dlt, ons = 0, chr = *str;
    uint8_t ns, b0, b1;

hashotloop:                          // a loop made by ASM jumps

    // 1. time deltas management ///////////////////////////////////////////////
#if USE_GET_TIME
    ts_tv_nsec = getnstime( NULL ) >> nbtls;
    dlt = ts_tv_nsec < ons ? 1E9 + ts_tv_nsec - ons : ts_tv_nsec - ons;
#else
    ts_tv_nsec = getnstime(&cpuid) >> nbtls;
    if( cpuid != oid ) {
          oid = cpuid;
          ons = ts_tv_nsec;
          goto reschedule;
    }
    oid = cpuid;
    dlt = ts_tv_nsec - ons;          // overflow by uint64_t is 0xff..ff + 1 = 0
#endif
    ns = 0xff & ts_tv_nsec;

    // 2. internal stats update ////////////////////////////////////////////////
    nstw = 0;
    if( ons ) {
        // dmn calculation is mandatory for stochastics biforkation turns
        if( dlt < dmn ) { ns *= 0x4d; dmn = dlt; }
        // dmx calculation can be simplified or omited but ns*=0x4d anyway
        if( dlt > dmx ) { ns *= 0x4d; dmx += (dlt ? dmx/dlt : 0); }
        nstw = dmn + nsdly + (pmdly ?  pmdly2ns : 0);
        // avg calculation can be omitted
        if( (dmn << 1) > dlt ) { avg += dlt; ncl++; }
    }

    // 3. entropy distillation /////////////////////////////////////////////////
    b0  = ohs;
    b1  = ons >> 3;
    ns ^= (ns >> 3) ^ b0 ^ (b1 << 2);
    b1  = ns & 0x02;
    b0  = ns & 0x01;

    // 4. nacro-mix in djb2-style //////////////////////////////////////////////
    /*
     * (16+1) (32-1 or 32+1) (64-1)
     *   01     10      00     11
     */
    hsh  = ( ( hsh << (4 + (b0 ? b1 : 1)) ) + (b1 ? -hsh : hsh) );

    // 5. entropy injection w/ rotation ////////////////////////////////////////
#if USE_PRIMES_2564
    hsh ^= chr ^ rotl64(chr, primes64[ns%10]);
#else
    hsh ^= chr ^ rotl64(chr, 1 + (ns & 0x07));
#endif

    // 6. stochastics micro-mix ////////////////////////////////////////////////
    hsh  = rotl64(hsh, 5 + ((ns >> 3) & 0x03)) + hsh;

    // 7. exceptions management ////////////////////////////////////////////////
    if( dlt < nstw || hsh == ohs ) { // copying with the VMs scheduler timings
#if USE_GET_TIME
#else
reschedule:
#endif
        str--;                       // apply changes but repeat the action
        nexp++;
        sched_yield();
        hsh *= 0x9E3779B9;           // Knuth, based on gold section
        goto hashotloop;             // continue made by an ASM jump
    }

    // 8. preparation for the next round ///////////////////////////////////////
    ohs = hsh;
    ons = ts_tv_nsec;
    if( (chr = *++str) && maxn-- ) {
        sched_yield();
        goto hashotloop;             // a loop made by two ASM jumps
    }

    // 9. finalising w/ a 32+1 bit mix /////////////////////////////////////////
    return (hsh * 0x45d9f3b) ^ (hsh >> 31);
}

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
//          buf[a] ^= (inp[a] << (8-(a&7))) | (inp[a] >> (a&7));
//          buf[a] ^= (a & 0x01) ? inp[a] : (inp[a] << 4) | (inp[a] >> 4);
//          buf[a] ^= inp[a]; // very simple alternative, to consider
//          buf[a] += inp[a]; // it creates a subtle mod5x3 at 2GB -> fails at 4GB
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

static inline void usage(const char *name) {
    perr("\n"\
"%s read on stdin, stats on stderr, and data on stdout\n"\
"\n"\
"Usage: %s [-h] [-tN] [-dN] [-pN] [-sN] [-rN]\n"\
"   -T: number of collision tests on the same input\n"\
"   -d: number of ns above min as the minimum delay\n"\
"   -p: number of parts as min/256 ns above the min\n"\
"   -s: number of bits to left shift on ns timings\n"\
"   -r: number of preliminary runs (default: 1)\n"\
"   -k: randomness injection in kernel by ioctl\n"\
"   -i: number of blocks to read from stdin\n"\
"\nWith -pN is suggested -r32+ for stats pre-evaluation\n\n", name, name);
    exit(0);
}

int main(int argc, char *argv[]) {
    struct rand_pool_info_buf entrnd;
    uint8_t *str = NULL, nbtls = 0, prsts = 0, quiet = 0, nblks = 1;
    uint32_t ntsts = 1, nsdly = 0;
    uint32_t nrdry = 1, pmdly = 0;
    int devfd = 0;

    // Collect arguments from optional command line parameters
    while (1) {
        int opt = getopt(argc, argv, "hT:s:d:p:r:k:i:q");
        if(opt == 'q') {
            quiet = 1;
        } else
        if(opt == '?' || opt == 'h') {
            usage("uchaos");
        } else
        if(opt == -1) {
            break;
        }

        if(!optarg) continue;

        long x = atoi(optarg);
        switch (opt) {
            // ABS sanitises the parametric inputs
            case 'T': ntsts = ABS(x); prsts = 1; break;
            case 's': nbtls = ABS(x); break;
            case 'd': nsdly = ABS(x); break;
            case 'r': nrdry = ABS(x); break;
            case 'p': pmdly = ABS(x); break;
            case 'i': nblks = ABS(x); break;
            case 'k': devfd = open(optarg, O_WRONLY); break;
        }
    }
    if (devfd < 0) {
        perror("open device");
        return EXIT_FAILURE;
    }
    if(quiet) prsts = 0;

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

    if(prsts) perr("\nRepetitions: ");
    for (uint32_t a = ntsts; a; a--) {
        // hashing
        uint64_t st = get_nanos();
        h = str2ht64(str, &h, &size, nsdly, pmdly, nbtls);
        mt += get_nanos() - st;

        // output
        if(!h) return EXIT_FAILURE;

        uint32_t sz = size << 3;
        if(devfd) {
            entrnd.buf_size = sz;
            // cautelatively 7 bits per byte
            entrnd.entropy_count = (sz << 3) - sz;
            memcpy((uint8_t *)entrnd.buf, (uint8_t *)h, sz);
            if (ioctl(devfd, RNDADDENTROPY, &entrnd) < 0) {
                perror("ioctl entrnd");
                return EXIT_FAILURE;
            }         // WARNING!!
        } /* else  */ // skip else expose the data but it is useful for debuging
        writebuf(STDOUT_FILENO, (uint8_t *)h, size << 3);

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
        sched_yield();   // Statistics are a block of CPU data-crunching but also
                         // a predictable delay which sched_yield() can jeopardise.
                         // Stats makes the large size output slower 1.7x than -q.
    }

    if(!prsts) return 0;

    uint64_t rt = get_nanos();
    perr("%s\n", nk ? ", status KO" : "none found, status OK");
    perr("\n");
    perr("Tests: %d w/ collisions %.0lf over %.1lf K hashes (%.2lf ppm)\n",
        ntsts, (double)nk, (double)nt/E3, (double)E6*nk/nt);

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
    perr("Times: running: %.3lf s, hashing: %.3lf s, speed: %.1lf Kh/s",
        (double)rt/E9, (double)mt/E9, (double)E6*nt/rt);
    uint32_t pmns = (uint32_t)djb2tum(0, 0, 0, 0, pmdly, 0);
    perr("Parameter settings: s(%d), d(%dns), p(%d:%dns), r(%d), RTSC(%d)\n",
        nbtls, nsdly, pmdly, pmns, nrdry, !USE_GET_TIME);

    perr("\n");

    return 0; // exit() do free()
}

