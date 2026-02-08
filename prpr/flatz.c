/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 license
 *
 * Usage: binary stream | flat [-p] [-q] [-zN]
 *
 * Compile with lib math: gcc flatz.c -O3 -ffast-math -lm -lz -o flatz
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

uint64_t djb2sum(const char *str, uint64_t seed) {
/*
 * One of the most popular and efficient hash functions for strings in C is
 * the djb2 algorithm created by Dan Bernstein. It strikes a great balance
 * between speed and low collision rates. Great for text.
 */
    uint64_t c, hash = 5381;
    /*
     * 5381              Prime number choosen by Dan Bernstein, as 1010100000101
     *                   empirically is one of the best for English words text.
     * Alternatives:
     *
     * 16777619               The FNV-1 offset basis (32-bit).
     * 14695981039346656037	  The FNV-1 offset basis (64-bit).
     */
    if(seed) hash = seed;
    if(!*str) return 0;
    while((c = *str++)) {
        // A slightly more aggressive mixing constant (31 or 33 are common)
        hash = ((hash << 5) + hash) ^ c;
    }

    return hash;
}

#ifdef __APPLE__
    #include <libkern/OSByteOrder.h>
    #define htole64(x) OSSwapHostToLittleInt64(x)
#elif defined(__linux__)
    #include <endian.h>
#else
    // Manual fallback for portability if not on Linux/macOS
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define htole64(x) __builtin_bswap64(x)
    #else
        #define htole64(x) (x)
    #endif
#endif

uint64_t fnv1sum(const void *data, size_t len) {
/*
 * While djb2 (5381) is legendary for strings, it was designed for ASCII text.
 * Compressed binary data contains many 0x00 (null) bytes and high-bit values
 * that djb2 can sometimes struggle to distribute evenly. Instead, FNV-1a was
 * specifically engineered for mapping byte sequences (like binary blobs) to
 * indices with minimal collisions.
 */
    const uint64_t *ptr = (const uint64_t *)__builtin_assume_aligned(data, 8);
    uint64_t hash = 14695981039346656037ULL; // The FNV-1 offset basis (64-bit).
    const uint64_t prime = 1099511628211ULL; // The 64-bit FNV prime

    for (size_t i = 0; i < len; i++) {
        hash ^= htole64(ptr[i]);
        hash *= prime;
    }

    return hash;
}

/**
 * @param data  Must be 8-byte aligned
 * @param n_64  Number of 64-bit blocks (len_bytes / 8)
 */
uint64_t fnv8sum(const uint64_t *data, size_t n_64) {
/*
 * This assumes data is 64bit memory aligned in address and size (0-padding).
 */
    // Hint to compiler: pointer is aligned to 8-byte boundary
    const uint64_t *ptr = (const uint64_t *)__builtin_assume_aligned(data, 8);
    uint64_t hash = 14695981039346656037ULL; // The FNV-1 offset basis (64-bit).
    const uint64_t prime = 1099511628211ULL; // The 64-bit FNV prime

    for (size_t i = 0; i < n_64; i++) {
        // Direct access is safe because of your alignment guarantee
        hash ^= htole64(ptr[i]);
        hash *= prime;
    }

    return hash;
}

/**
 * @param data  Must be 8-byte aligned and 0-padded to 64-bit boundaries.
 * @param n_64  Number of 64-bit blocks.
 */
uint64_t fnv64sum(const uint64_t *data, size_t n_64) {
    // Hint to compiler: pointer is aligned to 8-byte boundary
    const uint64_t *ptr = (const uint64_t *)__builtin_assume_aligned(data, 8);
    uint64_t hash = 14695981039346656037ULL; // The FNV-1 offset basis (64-bit).
    const uint64_t prime = 1099511628211ULL; // The 64-bit FNV prime

    size_t i = 0;

    // 1. Unrolled Hot Loop (Process 4 blocks at a time)
    for (; i < (n_64 & ~3); i += 4) {
        hash ^= htole64(ptr[i]);     hash *= prime;
        hash ^= htole64(ptr[i + 1]); hash *= prime;
        hash ^= htole64(ptr[i + 2]); hash *= prime;
        hash ^= htole64(ptr[i + 3]); hash *= prime;
    }

    // 2. Cleanup Loop (Process remaining 0-3 blocks)
    // Even with 0-padding, n_64 might not be a multiple of 4.
    for (; i < n_64; i++) {
        hash ^= htole64(ptr[i]);
        hash *= prime;
    }

    return hash;
}

static inline void print_hash(uint64_t hj, uint16_t hl) {
    for (uint16_t i = 0; i < hl; i++, hj >>= 8)
        perr("%01x%01x", ((uint8_t)hj) & 0x0F, ((uint8_t)hj) >> 4);
}

/* ************************************************************************** */

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

static inline void stats_print_head(const char *dscr, size_t size, double ratio) {
  perr("\n%s: %ld bytes, %.1lf Kb, %.3lf Mb", dscr,
      size, (double)size / (1<<10), (double)size / (1<<20));
  if(ratio > 0) {
    perr(", rtio: %lf %% (1 : %.3lf)\n", ratio * 100, 1.0 / ratio);
  } else {
    long nsrun = get_nanos();
    perr(", pid: %d, elab: %.1lf ms (%.1lf Kb/s)\n",
        getpid(), (double)nsrun / E6, (double)size * (E9 >> 10) / nsrun);
  }
}

/*
 * TODO: fare una stats_t as sharing struct
 */
unsigned printstats(const char *str, size_t nread, unsigned nsymb,
    uint32_t *counts, double ratio, bool head, bool rset)
{
    static char entdone = 0;
    static double entropy = 0, pavg = 0, avg = 0;
    double x = 0, px = 0, k = 0, s = 0, n = 0;
    size_t i;

    if(rset) { entdone = 0; entropy = 0; pavg = 0; avg = 0; n = 0; return 0; }

    double ex = ((double)nread) / nsymb, epx = 1.0 / nsymb;
    for (i = 0; i < 256; i++) {
        x = ((double)counts[i] - ex);
        s += x * x / ex;
        px = ((double)counts[i]) / nread;
        x = px - epx;
        k += x * x;

        if(!counts[i] || entdone) continue;
        entropy -= px * log2(px);
        avg += i * counts[i];
        n++;
    }
    if(!entdone) {
      entdone = 1;
      avg = avg / nread;
      pavg = (avg/AVGV - 1)*100;
    }

    double lg2s = log2(nsymb);

    if(head) stats_print_head(str, nread, ratio);
    fprintf(stderr,
        "%s: syml: %3ld, Eñ: %8.6lf / %4.2f = %4.2lf, X²: %10.3lf, k²: %9.5lf, avg: %9.5lf %+4g %%\n",
            str, MIN(nread,nsymb), entropy, lg2s, entropy/lg2s, s, k * nsymb, avg, pavg);

    return n;
}

#define st_t struct stats

typedef struct stats {
    /* -- 8-Byte Aligned Group -- */
    void    *pbuf;                // keep tract of the buffer address
    uint8_t *data;                // buffer pointer for data elaboration
    unsigned (*elab)(st_t *st);   // function pointer for block elaboration
    size_t   (*calc)(st_t *st);   // function pointer for total elaboration
    void     (*show)(st_t *st);   // function pointer for print stats' show
    double   avg;                 // average as 'ent' provides
    double   avg_sum;             // for progressive sum of data elements
    double   avg_exp;             // expected average (by type)
    double   avg_pdv;             // = %(avg - avg_exp)/avg_exp
    double   nbits;               // = log2(nsybl)
    double   entropy;             // entropy 8-bit based
    double   ent1bit;             // 1-bit entropy density
    double   x2;                  // Chi-square as 'ent' provides
    double   k2;                  // Squared mean freq. deviations
    double   ratio;               // sizes ratio compared to original dataset
    double   log2s;               // store the log2(nsybl) value, calculated once
    size_t   bsize;               // size in bytes of the elaboration block
    size_t   ntot;                // total size in bytes of the original dataset

    /* -- 4-Byte Aligned Group -- */
    uint32_t counts[256];         // array of frequencies (by counters)
    unsigned nsybl;               // n. of symbols found in the dataset
    unsigned nmax;                // = 1U << nencg, max n. of encodable symbols

    /* -- 1-Byte Aligned Group -- */
    char     name[6];             // a string for the name of dataset
    uint8_t  nenc;                // n. bits needed for encoding nsybl
    uint8_t  base;                // the standard base of counts (2^8 = 256)
} stats_t;

unsigned stats_block_elab(stats_t *st) {
    if (!st) return 0;

    // Localized variables for compiler optimisation
    // register keywords and while uusage for speed.
    register size_t   len = st->bsize;
    register uint8_t *d   = st->data;
    uint32_t         *c   = st->counts;
    double            sum = 0;

    if(!len || !d) return 0;

    // Updated before decrementing the value in len:
    st->ntot += len;

    while(len--) {
        uint8_t v = *d++; // equivalent to v=*d; d++
        sum += v;
        c[v]++;
    }

    // Updates struct values
    st->avg_sum += sum;
    st->avg = st->avg_sum / st->ntot;

    return st->ntot;
}

void stats_print_line(stats_t *st) {
    stats_print_head(st->name, st->ntot, st->ratio);
    perr("%s: symbl: %3ld, Eñ: %8.6lf / %4.2f = %5.1lf %%, X²: %8.2lf, k²: %7.4lf, avg: %8.7g %+6.4g %%\n",
        st->name, MIN(st->ntot, st->nmax), st->entropy, (double)st->nenc, (st->entropy * 100) / st->nenc,
        st->x2, st->k2 * st->nsybl, st->avg, st->avg_pdv);
    if(st->nsybl < 256) return;
    perr("%s: symbl: %3ld, Eñ: %8.6lf / %4.2f = %5.1lf %%, X²: %8.2lf, k²: %7.4lf, avg: %8.7g %+6.4g %%\n",
        st->name, MIN(st->ntot, st->nsybl), st->entropy, st->log2s, st->ent1bit * 100,
        st->x2, st->k2 * st->nsybl, st->avg, st->avg_pdv);
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

static inline void *ptralign(void *buf) {
    uintptr_t p = (uintptr_t)buf;
    return (void *)ALGN64(p);
}

#define writeout(b,s) { if(pass) { writebuf(STDOUT_FILENO, (const uint8_t *)(b), (size_t)(s)); } }

size_t zdeflating(const int action, uint8_t const *zbuf, z_stream *pstrm,
    uint32_t hsize, uint32_t tsize, uint32_t *zcounts, bool pass)
{
    static size_t tsved = 0, zsizetot = 0, zsize = 0;
    uint8_t *zbuffer;
    int i, ret;

    if(action != Z_NO_FLUSH && action != Z_FINISH)
        return 0;

    unsigned char *tbuffer = NULL;
    if (tsize > 0) {
        if (posix_memalign((void **)&tbuffer, 64, tsize)) {
            perror("posix_memalign");
            exit(EXIT_FAILURE);
        }
    }

    do {
        zbuffer = (uint8_t *)zbuf;
        pstrm->next_out = zbuffer;
        pstrm->avail_out = MAX_COMP_SIZE;
        ret = deflate(pstrm, action);
        if(ret !=  Z_OK && ret != Z_STREAM_END) {
            perror("zlib::deflateEnd");
            exit(EXIT_FAILURE);
        }
/* TODO: -hN -tN
        zsize = MAX_COMP_SIZE - pstrm->avail_out;
        if(hsize && zsize > 0) {
            if(zsize >= hsize) {
                zsize -= hsize;
                zbuffer += hsize;
                hsize = 0;
            } else {
                hsize -= zsize;
                zsize = 0;
            }
        }
        if(tsize > 0 && zsize > 0) {
            if(zsize >= tsize) {
                if(tsved > 0) {
                    writeout(tbuffer, tsved);
                    for (i = 0; i < tsved; i++) zcounts[ tbuffer[i] ]++;
                    zsizetot += tsved;
                    tsved = 0;
                 }
                 memcpy(tbuffer, zbuffer, tsize);
                 memmove(zbuffer, &zbuffer[tsize], tsize);
                 zsize -= tsize;
                 tsved = tsize;
            } else
            if(tsved >= zsize) {
                size_t nz = tsved-zsize;
                if(nz > 0) {
                    writeout(tbuffer, nz);
                    for (i = 0; i < nz; i++) zcounts[ tbuffer[i] ]++;
                    zsizetot += nz;
                    tsved -= nz;
                    memmove(tbuffer, &tbuffer[nz], tsved);
                    memcpy(&tbuffer[tsved], zbuffer, nz);
                    memmove(zbuffer, &zbuffer[nz], nz);
                    zsize -= nz;
                }
            }
        }
*/
    } while( (action == Z_NO_FLUSH && pstrm->avail_out == 0)
          || (action == Z_FINISH   && ret != Z_STREAM_END) );

    if(tbuffer) //  useless here/now but for best practice
        free(tbuffer);

    return MAX_COMP_SIZE - pstrm->avail_out;
}

#define Z_ON (zipl >= 0)
#define J_ON (jsize > 0)
#define P_ON (pass)

#define ZDEF(f) zs.bsize = zdeflating(f, zs.data, &strm, hsize, tsize, zs.counts, pass)
#define SHOW(s) { if(!quiet) { (void)stats_total_calc(s); stats_print_line(s); } }
#define PASS(x) { if(x) (void)writebuf(STDOUT_FILENO, outbuf, outsz); }
#define ELAB(s) { if(!quiet) (void)stats_block_elab(s); }
#define CALC(s) { if(!quiet) (void)stats_total_calc(s); }

static inline void usage(const char *name) {
    perr("\n"\
"%s read on stdin, stats on stderr, and data on stdout\n"\
"\n"\
"Usage: %s [-p] [-q] [-zN [-hN] [-tN]]\n"\
"   -q: no stats (quiet)\n"\
"   -p: data pass-through\n"\
"   -j: text hash (block N x 64bit, max:256)\n"\
"   -z: data compression (N:level, 0-9)\n"\
"   -h: skip header (N:bytes, max:256)\n"\
"   -t: skip tail (N:bytes, max:256)\n"\
"\n", name, name);
}

#if 0 /* ******************************************************************** */

static inline float fastlog2(float val) { // for numbers above 1.00, only!
    union { float f; uint32_t i; } vx = { val };
    register float y = (float)vx.i;
    y *= 1.1920928955078125e-7f;    //= 1 / 2^23
    return (y - 124.22551499f);     //* 1.442695f natural logaritm conv.;
}

#endif /* ******************************************************************* */

#include <stdalign.h>
alignas(64) const uint32_t tablog2[256] = {
0xff800000, 0x00000000, 0x3f800000, 0x3fcae00d, 0x40000000, 0x40149a78, 0x40257007, 0x4033abb4,
0x40400000, 0x404ae00d, 0x40549a78, 0x405d6754, 0x40657007, 0x406cd401, 0x4073abb4, 0x407a0a7f,
0x40800000, 0x4082cc7f, 0x40857007, 0x4087ef06, 0x408a4d3c, 0x408c8ddd, 0x408eb3aa, 0x4090c105,
0x4092b803, 0x40949a78, 0x40966a01, 0x4098280a, 0x4099d5da, 0x409b7495, 0x409d053f, 0x409e88c7,
0x40a00000, 0x40a16bad, 0x40a2cc7f, 0x40a42316, 0x40a57007, 0x40a6b3d8, 0x40a7ef06, 0x40a92204,
0x40aa4d3c, 0x40ab7111, 0x40ac8ddd, 0x40ada3f6, 0x40aeb3aa, 0x40afbd43, 0x40b0c105, 0x40b1bf31,
0x40b2b803, 0x40b3abb4, 0x40b49a78, 0x40b58482, 0x40b66a01, 0x40b74b20, 0x40b8280a, 0x40b900e6,
0x40b9d5da, 0x40baa709, 0x40bb7495, 0x40bc3e9d, 0x40bd053f, 0x40bdc89a, 0x40be88c7, 0x40bf45e1,
0x40c00000, 0x40c0b73d, 0x40c16bad, 0x40c21d67, 0x40c2cc7f, 0x40c37908, 0x40c42316, 0x40c4caba,
0x40c57007, 0x40c6130b, 0x40c6b3d8, 0x40c7527c, 0x40c7ef06, 0x40c88984, 0x40c92204, 0x40c9b892,
0x40ca4d3c, 0x40cae00d, 0x40cb7111, 0x40cc0053, 0x40cc8ddd, 0x40cd19bb, 0x40cda3f6, 0x40ce2c98,
0x40ceb3aa, 0x40cf3935, 0x40cfbd43, 0x40d03fdb, 0x40d0c105, 0x40d140ca, 0x40d1bf31, 0x40d23c42,
0x40d2b803, 0x40d3327c, 0x40d3abb4, 0x40d423b0, 0x40d49a78, 0x40d51012, 0x40d58482, 0x40d5f7d0,
0x40d66a01, 0x40d6db19, 0x40d74b20, 0x40d7ba19, 0x40d8280a, 0x40d894f7, 0x40d900e6, 0x40d96bdb,
0x40d9d5da, 0x40da3ee8, 0x40daa709, 0x40db0e41, 0x40db7495, 0x40dbda07, 0x40dc3e9d, 0x40dca259,
0x40dd053f, 0x40dd6754, 0x40ddc89a, 0x40de2914, 0x40de88c7, 0x40dee7b4, 0x40df45e1, 0x40dfa34e,
0x40e00000, 0x40e05bf9, 0x40e0b73d, 0x40e111cd, 0x40e16bad, 0x40e1c4e0, 0x40e21d67, 0x40e27546,
0x40e2cc7f, 0x40e32314, 0x40e37908, 0x40e3ce5e, 0x40e42316, 0x40e47734, 0x40e4caba, 0x40e51daa,
0x40e57007, 0x40e5c1d1, 0x40e6130b, 0x40e663b7, 0x40e6b3d8, 0x40e7036e, 0x40e7527c, 0x40e7a103,
0x40e7ef06, 0x40e83c85, 0x40e88984, 0x40e8d603, 0x40e92204, 0x40e96d88, 0x40e9b892, 0x40ea0323,
0x40ea4d3c, 0x40ea96df, 0x40eae00d, 0x40eb28c8, 0x40eb7111, 0x40ebb8e9, 0x40ec0053, 0x40ec474e,
0x40ec8ddd, 0x40ecd401, 0x40ed19bb, 0x40ed5f0c, 0x40eda3f6, 0x40ede879, 0x40ee2c98, 0x40ee7052,
0x40eeb3aa, 0x40eef6a0, 0x40ef3935, 0x40ef7b6b, 0x40efbd43, 0x40effebd, 0x40f03fdb, 0x40f0809d,
0x40f0c105, 0x40f10114, 0x40f140ca, 0x40f18029, 0x40f1bf31, 0x40f1fde4, 0x40f23c42, 0x40f27a4c,
0x40f2b803, 0x40f2f568, 0x40f3327c, 0x40f36f40, 0x40f3abb4, 0x40f3e7d9, 0x40f423b0, 0x40f45f3b,
0x40f49a78, 0x40f4d56a, 0x40f51012, 0x40f54a6f, 0x40f58482, 0x40f5be4d, 0x40f5f7d0, 0x40f6310c,
0x40f66a01, 0x40f6a2b0, 0x40f6db19, 0x40f7133f, 0x40f74b20, 0x40f782be, 0x40f7ba19, 0x40f7f132,
0x40f8280a, 0x40f85ea1, 0x40f894f7, 0x40f8cb0e, 0x40f900e6, 0x40f9367f, 0x40f96bdb, 0x40f9a0f9,
0x40f9d5da, 0x40fa0a7f, 0x40fa3ee8, 0x40fa7316, 0x40faa709, 0x40fadac2, 0x40fb0e41, 0x40fb4187,
0x40fb7495, 0x40fba76a, 0x40fbda07, 0x40fc0c6d, 0x40fc3e9d, 0x40fc7096, 0x40fca259, 0x40fcd3e7,
0x40fd053f, 0x40fd3664, 0x40fd6754, 0x40fd9810, 0x40fdc89a, 0x40fdf8f0, 0x40fe2914, 0x40fe5906,
0x40fe88c7, 0x40feb856, 0x40fee7b4, 0x40ff16e3, 0x40ff45e1, 0x40ff74af, 0x40ffa34e, 0x40ffd1be
}; const float *ptablog2 = (const float *)tablog2;

/*
 * NOTE: using ptablog2[] instead of log2f() is **educational** when running on
 *       modern CPU w/FPU because the log2f() is still competitive and hot-cache
 *       makes the difference between MEM/FPU competition. However, in others
 *       architecture like ESP32 S3, the table approach is the sole that can helps
 *       to speed-up entropy calculation. Because keeping in run a tool is the
 *       best way to be forced in maintaining it, for when it will be necessary,
 *       therefore I go with this pre-calculated table implementation. The other
 *       is pretty straighforwad: s/(ptablog2[ ci ] - log2_nread)/log2f(px)/
 */
 #define tablog2_ptr ptablog2

size_t stats_total_calc(stats_t *st) {
    if (!st) return 0;

    // Localized variables for compiler optimisation
    // register keywords and while uusage for speed.
    const size_t nread    = st->ntot;
    register size_t   len = st->ntot;
    register uint8_t *d   = st->data;
    uint32_t         *c   = st->counts;
    double            sum = 0;

    if(!len || !d || !st->avg_sum) return 0;

    // Filling the stats strucuture with precalculated values
    if(!st->nsybl) {
        register unsigned nsybl = 0;
        for (register int i = 0; i < 256; i++)
            if(c[i]) nsybl++;
        st->nsybl = nsybl;
        st->log2s = log2(nsybl); // run once per dataset, precision vs speed
    }

    if(!st->avg_pdv) st->avg_pdv = (st->avg/st->avg_exp - 1) * 100;

    double s = 0, k = 0, e = 0;
    const double epx = 1.0 / (1U << st->base);              // st->nsybl;
    const double ex = epx * nread;                          // st->nsybl;
    const double ex_inv = 1.0 / ex;
    const double nread_inv = 1.0 / nread;
    const float log2_nread = log2f(nread);

    for (register int i = 0; i < 256; i++) {
        register int ci = c[i];
        register double x  = - ex + ci ;
        s += (x * x) * ex_inv;                              // X² aka chi-square

        const double px = nread_inv * ci;
        x = px - epx;
        k += (x * x);                                       // k² aka RMS freq. dev.
#ifdef tablog2_ptr
        if(ci) e -= px * (ptablog2[ ci ] - log2_nread);     // entropy
#else                                                       // | equal within 1ppm.
        if(ci) e -= px * log2f(px);                         // entropy
#endif
    }

    st->x2 = s;
    st->k2 = k;
    st->entropy = e;
    st->ent1bit = e / st->log2s;
    st->nenc = ceil(st->log2s);
    st->nmax = 1U << st->nenc;

    return st->ntot;
}

int main(int argc, char *argv[]) {
    z_stream strm = {0};
    int pass = 0, zipl = -1, quiet = 0;
    size_t i, hsize = 0, tsize = 0, jsize = 0;
    stats_t rs = {0}, js = {0}, zs = {0};

    (void) get_nanos(); //----------------------------------------------------//
    /*
     * TODO: write a function that create and initialise such a structure
     */
    // Static memory allocation: it fails immediately or it runs forever
    unsigned char rbuf[MAX_READ_SIZE+64];
    unsigned char jbuf[MAX_READ_SIZE+64];
    unsigned char zbuf[MAX_COMP_SIZE+64];

#ifdef print_tablog2_func
    print_tablog2_func();
#endif

    // Zeroing the structures, best practice only: given zeroed for security
    memset(&rs, 0, sizeof(rs));
    memset(&js, 0, sizeof(js));
    memset(&zs, 0, sizeof(zs));

    // Memory alignment at 64 bit: more an attitude than an optimisation
    rs.pbuf = (void *)ptralign(rbuf);
    js.pbuf = (void *)ptralign(jbuf);
    zs.pbuf = (void *)ptralign(zbuf);

    // Stats structure initialisation
    snprintf(rs.name, sizeof(rs.name), "rdata");
    snprintf(js.name, sizeof(js.name), "jdata");
    snprintf(zs.name, sizeof(zs.name), "zdata");
    rs.data = (uint8_t *)rs.pbuf;
    js.data = (uint8_t *)js.pbuf;
    zs.data = (uint8_t *)zs.pbuf;
    rs.elab = stats_block_elab;
    js.elab = stats_block_elab;
    zs.elab = stats_block_elab;
    rs.calc = stats_total_calc;
    js.calc = stats_total_calc;
    zs.calc = stats_total_calc;
    rs.show = stats_print_line;
    js.show = stats_print_line;
    zs.show = stats_print_line;
    rs.avg_exp = AVGV;
    js.avg_exp = AVGV;
    zs.avg_exp = AVGV;
    rs.base = 8;
    js.base = 8;
    zs.base = 8;

    // Collect arguments from optional command line parameters
    while (1) {
        int opt = getopt(argc, argv, "pqz:h:t:j:");
        if(opt == '?' && !optarg) {
          usage("flatz"); exit(0);
        } else if(opt == -1) break;

        switch (opt) {
            case 'p': pass = 1; break;
            case 'q': quiet = 1; break;
            case 'z': zipl = atoi(optarg); break;
            case 'h': hsize = atoi(optarg); break;
            case 't': tsize = atoi(optarg); break;
            case 'j': jsize = atoi(optarg); break;
        }
    }

    // Sanitise the optional argument
    zipl  = MAX(-1, zipl);
    hsize = MAX(0, hsize);
    tsize = MAX(0, tsize);
    jsize = MAX(0, jsize);
    hsize = MIN(hsize, 256);
    tsize = MIN(tsize, 256);
    jsize = MIN(jsize, 256);

    // libz initialisation
    if (Z_ON) {
        strm.next_out = zbuf;
        strm.avail_out = MAX_COMP_SIZE;
        if (deflateInit(&strm, zipl) != Z_OK) {
            perror("libz::deflateInit");
            exit(EXIT_FAILURE);
        }
    }

    // Aesthetic blankline
    if(!quiet) perr("\n");

    #if 0  // --------------------------------------------------------------- //
    #define BLOCK_SIZE MAX_READ_SIZE
    #else
    #define BLOCK_SIZE 64
    #endif

    // decoupling output from storage, uniforming API output
    size_t outsz;
    uint8_t *outbuf;
    #define setout(b,s) { outbuf = (uint8_t *)(b); outsz = (size_t)(s); }

    for (unsigned k = 0; true; k) { //-- service loop start --------------- --//
        static uint64_t hash;

        // read data from input stream
        outsz = (J_ON) ? jsize : BLOCK_SIZE;
        rs.bsize = readbuf(STDIN_FILENO, rs.data, outsz, 0);
        if(!rs.bsize) break;

        setout(rs.data, rs.bsize);
        ELAB(&rs);

        // write stdin stream on stdout, if requested
        if (J_ON) {
            hash = djb2sum(outbuf, 0);
            setout(&hash, sizeof(hash));
        }
        if (Z_ON) {
            strm.avail_in = outsz;
            strm.next_in = outbuf;
            ZDEF(Z_NO_FLUSH);
            //TODO
            //setout(zs.data, zs.bsize);
            //ELAB(&zs);
        }
        else PASS(P_ON);

        if(quiet) continue;

        perr("DGB, rs(%03d)> avg: %7.3lf, ntot: %4ld, bsize: %4ld",
            ++k, rs.avg, rs.ntot, rs.bsize);
        if (J_ON) { perr(", djb2: "); print_hash(hash, 8); }
        perr("\n");
    } //-- service while end ---------------------------------------------- --//
#if 1
    SHOW(&rs); // Show read data statistics, if not inhibited
#else
    CALC(&rs);
    printstats(rs.name, rs.ntot, 256, rs.counts, 0, 1, 0);
    printstats(0, 0, 0, 0, 0, 0, 1);
#endif

    // Finalise the zlib compression process
    if (Z_ON) {
        ZDEF(Z_FINISH);
        if(deflateEnd(&strm) != Z_OK) {
            perror("zlib::deflateEnd");
            exit(EXIT_FAILURE);
        }
        setout(zs.data, zs.bsize);
        PASS(P_ON);
        ELAB(&zs);
        zs.ratio = (double)zs.ntot / rs.ntot;
#if 1
        SHOW(&zs); // Show libz data statistics, if not inhibited
#else
        CALC(&zs);
        printstats(zs.name, zs.ntot, 256, zs.counts, zs.ratio, 1, 0);
        printstats(0, 0, 0, 0, 0, 0, 1);
#endif
    }

    if(!quiet) perr("\n");

    return 0;
}
