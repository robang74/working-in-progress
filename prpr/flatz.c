/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 license
 *
 * Usage: binary stream | flat [-p] [-q] [-zN]
 *
 * Compile with lib math: gcc flatz.c -O3 -lm -lz -o flatz
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

#define AVGV 125.5
#define E3 1000L
#define E6 1000000L
#define E9 1000000000L
#define MAX_READ_SIZE 4096
#define MAX_COMP_SIZE (MAX_READ_SIZE << 1)
#define ABS(a)   ( ( (a) < 0 )  ? -(a) : (a) )
#define MIN(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#define MAX(a,b) ( ( (a) > (b) ) ? (a) : (b) )

/* *** HASHING  ************************************************************* */

uint64_t djb2sum(const char *str, uint64_t seed) {
/*
 * One of the most popular and efficient hash functions for strings in C is
 * the djb2 algorithm created by Dan Bernstein. It strikes a great balance
 * between speed and low collision rates. Great for text.
 */
    uint64_t hash;
    int c;

    if(!seed) hash = 5381;
    /*
     * 5381              Prime number choosen by Dan Bernstein, as 1010100000101
     *                   empirically is one of the best for English words text.
     * Alternatives:
     *
     * 16777619               The FNV-1 offset basis (32-bit).
     * 14695981039346656037	  The FNV-1 offset basis (64-bit).
     */
    while ((c = *str++)) {
        // A slightly more aggressive mixing constant (31 or 33 are common)
        hash = ((hash << 5) + hash) ^ (uint64_t)c;
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

/*
 * TODO: fare una stats_t as sharing struct
 */
unsigned printstats(const char *str, size_t nread, unsigned nsymb,
    uint32_t *counts, bool idnt, bool rset)
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
    fprintf(stderr,
        "%s%s: %3ld, Eñ: %8.6lf / %4.2f = %8.6lf, X²: %10.3lf, k²: %9.5lf, avg: %9.5lf %+.4lf %%\n",
        idnt?"  ":"", str, MIN(nread,nsymb), entropy, lg2s, entropy/lg2s, s, k * nsymb, avg, pavg);

    return n;
}

typedef struct {
    /* -- 8-Byte Aligned Group -- */
    void    *pbuf;                // keep tract of the buffer address
    uint8_t *data;                // buffer pointer for data elaboration
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
    size_t   bsize;               // size in bytes of the elaboration block
    size_t   ntot;                // total size in bytes of the original dataset

    /* -- 4-Byte Aligned Group -- */
    uint32_t counts[256];         // array of frequencies (by counters)
    unsigned nsybl;               // n. of symbols found in the dataset
    unsigned nmax;                // = 1 << nencg, max n. of encodable symbols

    /* -- 1-Byte Aligned Group -- */
    char     name[7];             // a string for the name of dataset
    uint8_t  nencg;               // n. bits needed for encoding nsybl
} stats_t;

void printallstats(const char *dstr, const unsigned char *buf, size_t size,
    uint32_t *counts, double ratio, bool rset)
{
    if(rset) {
        printstats(0, 0, 0, 0, 0, 1);
        memset(counts, 0, sizeof(size_t) << 8);
        for (size_t i = 0; i < size; i++)
            counts[ buf[i] ]++;
    }

    fprintf(stderr, "\n");
    if (size > 256) {
        fprintf(stderr, "%s: %ld bytes, %.1lf Kb, %.3lf Mb", dstr,
            size, (double)size / (1<<10), (double)size / (1<<20));
        if(ratio > 0) {
          fprintf(stderr, ", rtio: %lf %% (1 : %.3lf)\n", ratio * 100, 1.0 / ratio);
        } else {
          long nsrun = get_nanos();
          fprintf(stderr, ", pid: %d, elab: %.1lf ms (%.1lf Kb/s)\n",
              getpid(), (double)nsrun / E6, (double)size * (E9 >> 10) / nsrun);
        }
    }
    unsigned nsymb = printstats("bytes", size, 256, counts, 1, 0);
    double lg2s = log2(nsymb);
    unsigned nbits = ceil(lg2s), nmax = 1 << nbits;
    if (nbits < 8)
        (void)printstats("encdg", size, nmax, counts, 1, 0);
    if(nsymb < nmax)
        (void)printstats("symbl", size, nsymb, counts, 1, 0);
}

static inline ssize_t writebuf(int fd, const char *buffer, size_t ntwr) {
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

#define ALGN64(n) ( ( ( (n) + 63) >> 6 ) << 6 )

static inline void *memalign(void *buf) {
    uintptr_t p = (uintptr_t)buf;
    return (void *)ALGN64(p);
}

size_t zdeflating(const int action, uint8_t const *zbuf, z_stream *pstrm,
    uint32_t hsize, uint32_t tsize, uint32_t *zcounts, bool pass)
{
    static size_t tsved = 0, zsizetot = 0;
    uint8_t *zbuffer;
    int i, ret;

    if(action != Z_NO_FLUSH && action != Z_FINISH)
        return 0;

    unsigned char *tbuf, *tbuffer = NULL;
    if (tsize > 0) {
        tbuf = malloc(tsize+64);
        if (!tbuf) {
          perror("malloc");
          exit(EXIT_FAILURE);
        }
        tbuf = (unsigned char *)memalign(tbuf);
        tbuffer = tbuf;
    }

    do {
        zbuffer = (uint8_t *)zbuf;
        pstrm->next_out = zbuffer;
        pstrm->avail_out = MAX_COMP_SIZE;
        ret = deflate(pstrm, action);
        size_t zsize = MAX_COMP_SIZE - pstrm->avail_out;
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
                    writebuf(STDOUT_FILENO, (const uint8_t *)tbuffer, tsved);
                    for (i = 0; i < tsved; i++) zcounts[ tbuf[i] ]++;
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
                    writebuf(STDOUT_FILENO, (const uint8_t *)tbuffer, nz);
                    for (i = 0; i < nz; i++) zcounts[ tbuf[i] ]++;
                    zsizetot += nz;
                    tsved -= nz;
                    memmove(tbuffer, &tbuffer[nz], tsved);
                    memcpy(&tbuffer[tsved], zbuffer, nz);
                    memmove(zbuffer, &zbuffer[nz], nz);
                    zsize -= nz;
                }
            }
        }
        if(zsize > 0) {
            if(pass)
                writebuf(STDOUT_FILENO, (const uint8_t *)zbuffer, zsize);
            for (i = 0; i < zsize; i++) zcounts[ zbuf[i] ]++;
            zsizetot += zsize;
        }
    } while( (action == Z_NO_FLUSH && pstrm->avail_out == 0)
          || (action == Z_FINISH   && ret != Z_STREAM_END) );

    free(tbuf); // TODO: use a fixed allocated buffer
    return zsizetot;
}

#define Z_ON (zipl >= 0)
#define J_ON (jsize > 0)
#define P_ON (pass && !Z_ON && !J_ON)
#define perr(x...) fprintf(stderr, x)

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

int main(int argc, char *argv[]) {
    z_stream strm = {0};
    int pass = 0, zipl = -1, quiet = 0;
    size_t i, hsize = 0, tsize = 0, jsize = 0;
    stats_t rs = {0}, js = {0}, zs = {0};

/*
    size_t rsizetot = 0, zsizetot = 0, jsizetot = 0;
    size_t rcounts[256] = {0}, zcounts[256] = {0}, jcounts[256] = {0};
    size_t i, nr = 0, nsved = 0;
*/

    (void) get_nanos(); //----------------------------------------------------//

    // Static memory allocation: it fails immediately or it runs forever
    unsigned char *rbuffer, rbuf[MAX_READ_SIZE+64];
    unsigned char *jbuffer, jbuf[MAX_READ_SIZE+64];
    unsigned char *zbuffer, zbuf[MAX_COMP_SIZE+64];

    // Memory alignment at 64 bit: more an attitude than an optimisation
    rs.pbuf = (void *)memalign(rbuf);
    js.pbuf = (void *)memalign(jbuf);
    zs.pbuf = (void *)memalign(zbuf);

    // Stats structure initialisation
    rs.data = (uint8_t *)rs.pbuf;
    js.data = (uint8_t *)js.pbuf;
    zs.data = (uint8_t *)zs.pbuf;
    snprintf(rs.name, 7, "rdata");
    snprintf(js.name, 7, "jdata");
    snprintf(zs.name, 7, "zdata");

    // TODO: temporary trick to make it compile
    #define rsizetot rs.ntot
    #define jsizetot js.ntot
    #define zsizetot zs.ntot
    #define rcounts  rs.counts
    #define jcounts  js.counts
    #define zcounts  zs.counts
    
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
            perror("deflateInit");
            exit(EXIT_FAILURE);
        }
    }

    // Aesthetic blankline
    if(jsize > 0) perr("\n");

//-- ---------------------------------------------------------------------- --//
#if 0
#define BLOCK_SIZE MAX_READ_SIZE
#else
#define BLOCK_SIZE 64
#endif

    // read data from input stream
    rs.bsize = readbuf(STDIN_FILENO, rs.data, BLOCK_SIZE, 0);
    // write stdin stream on stdout, if requested
    if(P_ON) { (void)writebuf(STDOUT_FILENO, rs.data, rs.bsize); }
    else
    if(J_ON) {}
    else // TODO: J_ON doesn't exclude Z_ON, simplification to remove later
    if(Z_ON) {}
    
unsigned stats_block_elab(stats_t *st) {
    //memset(st->counts, 0, sizeof(st->counts[0]) << 8);
    for (size_t i = 0; i < st->bsize; i++) {
        st->counts[ st->data[i] ]++;
        st->avg_sum += st->data[i];
    }
    st->ntot += st->bsize;
    st->avg = st->avg_sum / st->ntot;
    return 0;
}
    stats_block_elab(&rs);
    perr("DGB, rs.stats> avg: %lf, ntot: %ld, bsize: %ld\n", rs.avg, rs.ntot, rs.bsize);

//-- ---------------------------------------------------------------------- --//
#if 0
    int k = 0;
    while (1) {
        k++;
        // read input stream
        if(jsize > 0) {
            nr = readbuf(STDIN_FILENO, rbuffer, jsize << 3);
            if (nr == 0) break;
            rbuffer[nr] = 0; // String termination
            uint64_t hj = djb2sum(rbuffer, 0);
            if(!quiet) {
                perr("%03d: djb2sum(%03ld): ", k, nr);
                for (size_t i = 0; i < 8; i++, hj >>= 8) {
                    perr("%01x%01x", (uint8_t)hj & 0x0F, ((uint8_t)hj & 0xF0) >> 4);
                }
                perr("\n");
            }
            *(uint64_t *)jbuffer = hj; jbuffer += 8; jsizetot += 8;
        } else {
            nr = read(STDIN_FILENO, rbuffer, MAX_READ_SIZE);
            if (nr == 0) break;
            if (nr < 0) {
                if (errno == EINTR) continue;
                perror("read");
                exit(EXIT_FAILURE);
            }
        }
        // increase the read sise
        rsizetot += nr;
        // write stdin stream on stdout
        if(pass && !Z_ON) writebuf(STDOUT_FILENO, rbuffer, nr);
        // check for zip
        if (!Z_ON) continue;
        // z-compressing
        strm.avail_in = nr;
        strm.next_in = rbuffer;
        zdeflating(Z_NO_FLUSH, zbuf, &strm, hsize, tsize, zcounts, pass);
    }
    // pointers reset
    rbuffer = rbuf;
    jbuffer = jbuf;
    zbuffer = zbuf;

    if(!quiet) {
        printallstats("rdata", rbuf, rsizetot, rcounts, 0, 1);
        if(jsizetot > 0)
            printallstats("jdata", jbuf, jsizetot, jcounts,
                (double)jsizetot / rsizetot, 1);
    }
#endif
    if (Z_ON) {
        zsizetot = zdeflating(Z_FINISH, zbuf, &strm, hsize, tsize, zcounts, pass);
        deflateEnd(&strm);
        if(!quiet)
            printallstats("zdata", zbuf, zsizetot, zcounts,
                (double)zsizetot / rsizetot, 1);
    }

    if(!quiet) {
      fprintf(stderr, "\n");
      fflush(stderr);
    }
    fflush(stdout);

    return 0;
}
