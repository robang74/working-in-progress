/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 license
 *
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
uint64_t djb2tum(const char *str, uint64_t seed, uint8_t maxn) {
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

uint64_t *str2ht64(uint8_t *str, size_t *size) {
    if (!str || !size) return NULL;

    size_t n = strlen((char *)str);
    if (n == 0) return NULL;

    // 1. Determine rotation offset using monotonic time
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    size_t k = ts.tv_nsec % n;

    // 2. Calculate padding and allocation
    // We need enough 64-bit blocks to cover n bytes.
    size_t num_blocks = (n + 7) >> 3;
    size_t total_bytes = num_blocks << 3;
    *size = num_blocks;

    // Allocate aligned memory for the processed string
    uint8_t *rotated_str = NULL;
    if (posix_memalign((void **)&rotated_str, 64, total_bytes)) {
        perror("posix_memalign");
        return NULL;
    }
    // Initialize with zeros for automatic padding
    memset(rotated_str, 0, total_bytes);

    // 3. Perform rotation into the new buffer
    // Copy from k to end
    memcpy(rotated_str, str + k, n - k);
    // Copy from beginning to k
    if (k > 0) {
        memcpy(rotated_str + (n - k), str, k);
    }

    // 4. Generate the uint64_t array
    // Note: We allocate a separate array for hashes if that was the intent,
    // or we cast the rotated string. Based on your code, you want a hash per 8-byte block.
    if(posix_memalign(&h, 64, num_blocks << 3)) {
        perror("posix_memalign");
        free(rotated_str);
        return NULL;
    }

    for (size_t i = 0; i < num_blocks; i++) {
        // Process each 8-byte chunk of the rotated/padded string
        h[i] = djb2tum(rotated_str + (i << 3), 0, 8);
    }

    free(rotated_str);
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

static inline ssize_t readbuf(int fd, char *buffer, size_t size, bool intr) {
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

int main () {
    size_t size;
    uint64_t hash;
    unsigned char *str = NULL;
    if (tsize > 0) {
        if (posix_memalign((void **)&str, 512, tsize)) {
            perror("posix_memalign");
            exit(EXIT_FAILURE);
        }
    }

    n = readbuf(STDIN_FILENO, str, 512, 1);
    hash = str2ht64(str, &size);
    if(hash)
      writebuf(STDOUT_FILENO, hash, size << 3);

    return 0;
}
