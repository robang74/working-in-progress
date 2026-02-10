/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 license
 *
 * test: time for i in $(seq 100); do cat uchaos.c | ./chaos; done | ent
 *
 * Compile with lib math: gcc uchaos.c -O3 -Wall -o uchaos
 */

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

/* *** HASHING  ************************************************************* */

static inline uint64_t rotl64(uint64_t n, uint8_t c) {
    c &= 63; return (n << c) | (n >> ((-c) & 63));
}

#include <sched.h>
uint64_t djb2tum(const uint8_t *str, uint64_t seed, uint8_t maxn) {
    if(!str) return 0;
    if(!*str || !maxn) return 0;
/*
 * One of the most popular and efficient hash functions for strings in C is
 * the djb2 algorithm created by Dan Bernstein. It strikes a great balance
 * between speed and low collision rates. Great for text.
 */
    uint64_t c, h = 5381;
    /*
     * 5381              Prime number choosen by Dan Bernstein, as 1010100000101
     *                   empirically is one of the best for English words text.
     * Alternatives:
     *
     * 16777619               The FNV-1 offset basis (32-bit).
     * 14695981039346656037	  The FNV-1 offset basis (64-bit).
     */
    if(seed) h = seed;

    while((c = *str++) && maxn--) {
        struct timespec ts;                   //
        clock_gettime(CLOCK_MONOTONIC, &ts);  // getting ns in a hot loop is the limit
        sched_yield();                        // and we want to see this limit, in VMs


        uint8_t ns = ts.tv_nsec & 0xff;
        uint8_t b1 = ns & 0x02;
        uint8_t b0 = ns & 0x01;
        /*
         * (16+1) (32-1 or 32+1) (64-1)
         *   01     10      00     11
         */
        // 1. nacro-mix in djb2-style
        h  = ( ( h << (4 + (b0 ? b1 : 1)) ) + (b1 ? -h : h) );

        // 2. char injection w/ rotated
        h ^= c ^ rotl64(c, 1 + (ns & 0x07));

        // 3. stochastics micro-mix
        h  = rotl64(h, 5 + ((ns >> 3) & 0x03)) + h;
    }

    return h;
}

# define rotated_str_free { free(rotated_str); rotated_str = NULL; rotated_str_len = 0; }

uint64_t *str2ht64(uint8_t *str, uint64_t **ph,  size_t *size) {
    static uint8_t *rotated_str = NULL;
    static size_t rotated_str_len = 0;

    if (!str && !size) rotated_str_free;
    if (!str || !size) return NULL;

    size_t n = strlen((char *)str);
    if (n == 0) return NULL;

    // 1. Determine rotation offset using monotonic time
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    size_t k = ts.tv_nsec % n;

    // 2. Calculate padding and allocation
    // We need enough 64-bit blocks to cover n bytes.
    uint64_t *h = NULL;
    size_t num_blocks = (n + 7) >> 3;
    size_t total_bytes = num_blocks << 3;
    if(ph) {
        if(*size > 0 && num_blocks != *size) {
            perror("*size != expected");
            return NULL;
        }
        h = *ph;
    }

    // Allocate aligned memory for the processed string
    if(rotated_str && rotated_str_len < total_bytes)
        rotated_str_free;
    if (!rotated_str) {
        if (posix_memalign((void **)&rotated_str, 64, total_bytes)) {
            perror("posix_memalign");
            return NULL;
        }
        rotated_str_len = total_bytes;
    }

    // 3. Perform rotation into the new buffer
    // Copy from k to end
    memcpy(rotated_str, str + k, n - k);
    // Copy from beginning to k
    if (k > 0) {
        memcpy(rotated_str + (n - k), str, k);
    }
    // Padding with zeros
    for(size_t i = n; i < total_bytes; i++)
        rotated_str[i] = 0;

    // 4. Generate the uint64_t array
    // We allocate a separate array for hashes if that was the intent, or we cast
    // the rotated string. Based on your code, you want a hash per 8-byte block.
    if(!h) {
        if(posix_memalign((void **)&h, 64, num_blocks << 3)) {
            perror("posix_memalign");
            return NULL;
        }
    }
    for (size_t i = 0; i < num_blocks; i++) {
        // Process each 8-byte chunk of the rotated/padded string
        h[i] = djb2tum(rotated_str + (i << 3), 0, 8);
    }

    *size = num_blocks;
    return h;
}

static inline ssize_t writebuf(int fd, const uint8_t *buffer, size_t ntwr) {
    ssize_t tot = 0;
    while (ntwr > tot) {
        errno = 0;
        ssize_t nw = write(fd, buffer + tot, ntwr - tot);
        if (nw < 1) {
            if (errno == EINTR) continue;
            perror("write");
            exit(EXIT_FAILURE);
        }
        tot += nw;
   }
   return tot;
}

static inline ssize_t readbuf(int fd, uint8_t *buffer, size_t size, bool intr) {
    ssize_t tot = 0;
    while (size > tot) {
        errno = 0;
        ssize_t nr = read(fd, buffer + tot, size - tot);
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

// Funzione per ottenere il tempo in nanosecondi
long get_nanos() {
    static long start = 0;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (!start) {
      start = (long)ts.tv_sec * 1000000000L + ts.tv_nsec;
      return start;
    }
    return ((long)ts.tv_sec * 1000000000L + ts.tv_nsec) - start;
}

static inline void usage(const char *name) {
    perr("\n"\
"%s read on stdin, stats on stderr, and data on stdout\n"\
"\n"\
"Usage: %s [-tN]\n"\
"   -t: number of collision tests on the same input\n"\
"\n", name, name);
    exit(0);
}

#define BLOCK_SIZE 512

int main(int argc, char *argv[]) {
    uint8_t *str = NULL;
    uint32_t ntsts = 0;

    (void) get_nanos();

    // Collect arguments from optional command line parameters
    while (1) {
        int opt = getopt(argc, argv, "ht:");
        if(opt == '?' && !optarg) {
            usage("uchaos");
        } else if(opt == -1) break;

        switch (opt) {
            case 't': ntsts = atoi(optarg); break;
        }
    }

    if (posix_memalign((void **)&str, 64, BLOCK_SIZE)) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }

    size_t n = readbuf(STDIN_FILENO, str, BLOCK_SIZE - 1, 1);
    if(n < 1) exit(EXIT_FAILURE);
    str[n] = 0;

    long mt = 0;
    uint64_t bic = 0;
    uint64_t *h = NULL;
    size_t nk = 0, nt = 0, nx = 0, size = 0;
    for (uint32_t a = ntsts; a; a--) {
        // hashing
        long st = get_nanos();
        h = str2ht64(str, &h, &size);
        mt += get_nanos() - st;

        // output
        if(h)
            writebuf(STDOUT_FILENO, (uint8_t *)h, size << 3);
        else
            exit(EXIT_FAILURE);
        // single run
        if(!ntsts) return 0;

        // testing
        for (size_t n = 0; n < size; n++) {
            for (size_t i = n + 1; i < size; i++) {
                if (h[i] == h[n]) {
                    nk++; continue;
                }
                uint64_t cb = h[i] ^ h[n];
                for (int a = 0; a < 64; a++)
                    bic += (cb >> a) & 0x01;
                nx++;
            }
        }
        nt += size;
#if 0                      // Just for test
        free(h); h = NULL; // passing to str2ht64 a valid (h, size) should reused it
#endif
    }

    long rt = get_nanos();
    perr("\nTests: %d, collisions: %ld over %ld hashes (%.2lf%%) -- %s\n",
        ntsts, nk, nt, (double)100*nk/nt, nk?"KO":"OK");
    perr("\nTimes: running: %.3lf s, hashing: %.3lf s, speed: %.1lf Kh/s\n",
        (double)rt/E9, (double)mt/E9, (double)E6*nt/rt);
/*
    perr("\nBit in common: %ld on %ld, compared to 50%% avg is %lf %%\n",
        bic, nx << 6, (double)100 / 64 * bic / nx);
 */
    double ratio = (double)100 / 64 * bic / nx;
    perr("\nBits in common compared to 50 %% avg is %.4lf %% (%+.1lf ppm)\n",
        ratio, (ratio-50) * E6 / 100);
    perr("\n");

    str2ht64(0, 0, 0);      // pedantic free()
    return 0;               // exit() do free()
}

