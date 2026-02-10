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
#include <zlib.h>

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
uint64_t djb2tum(const char *str, uint64_t seed) {
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
    if(!*str) return 0;

    while((c = *str++)) {
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

uint64_t * str2ht64(uint8_t *str, size_t *size) {
    if(!str || !size) return NULL;

    size_t n = strlen(str);
    if(!n) return NULL;

    uint8_t r = n & 3;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    uint8_t *s = NULL;
    if (posix_memalign((void **)&s, 64, n + 8 - r)) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }

    size_t k = ts % n;
    for (uint8_t *p = str[k]; p; p++, s++) *s = *p;
    for (p = str; p && k; p++, k--) *s = *p;
    for (r; r; r--, p++) *p = 0;

    uint64_t *h = s;
    n = (n + 63) >> 6;
    *size = n;
    for (p = s, k = 0; n; n--) {
        for(uint8_t i = 0; i < 8; i++) str[i] = p[i];
        str[0]; h[k++] = djb2tum(str, 0); p += 8;
    }

    return h;
}

