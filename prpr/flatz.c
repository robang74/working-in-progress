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
        "%s: %3ld, Eñ: %8.6lf / %4.2f = %8.6lf, X²: %10.3lf, k²: %9.5lf, avg: %9.5lf %+4g %%\n",
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
/*
    perr("\n%s: %ld bytes, %.1lf Kb, %.3lf Mb", st->name,
        st->ntot, (double)st->ntot / (1<<10), (double)st->ntot / (1<<20));
    if(st->ratio > 0) {
      perr(", rtio: %lf %% (1 : %.3lf)\n", st->ratio * 100, 1.0 / st->ratio);
    } else {
      long nsrun = get_nanos();
      perr(", pid: %d, elab: %.1lf ms (%.1lf Kb/s)\n",
          getpid(), (double)nsrun / E6, (double)st->ntot * (E9 >> 10) / nsrun);
    }
*/
    perr("%s: %3ld symbl, Eñ: %5.3lf / %4.2f = %6.4lf, X²: %8.2lf, k²: %7.4lf, avg: %8.7g %+.4g %%\n",
        st->name, MIN(st->ntot, st->nsybl), st->entropy, st->log2s, st->ent1bit,
        st->x2, st->k2 * st->nsybl, st->avg, st->avg_pdv);
/*
        if (st->nenc < 8)
            (void)printstats("encdg", size, st->nmax, st->counts, 1, 0);
        if(st->nsybl < nmax)
            (void)printstats("symbl", size, st->nsybl, st->counts, 1, 0);
*/
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
        if(zsize > 0) {
            writeout(zbuffer, zsize);
            for (i = 0; i < zsize; i++) zcounts[ zbuffer[i] ]++;
            zsizetot += zsize;
        }
    } while( (action == Z_NO_FLUSH && pstrm->avail_out == 0)
          || (action == Z_FINISH   && ret != Z_STREAM_END) );

    if(tbuffer) //  useless here/now but for best practice
        free(tbuffer);

    return zsize;
}

#define Z_ON (zipl >= 0)
#define J_ON (jsize > 0)
#define P_ON (pass && !Z_ON)

#define SHOW(s) { if(!quiet) { (void)stats_total_calc(s); stats_print_line(s); } }
#define ZDEF(f) zs.bsize = zdeflating(f, zs.data, &strm, hsize, tsize, zs.counts, pass)
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

static inline float fastlog2(float val) {
    union { float f; uint32_t i; } vx = { val };
    register float y = (float)vx.i;
    y *= 1.1920928955078125e-7f;    //= 1 / 2^23
    return y - 124.22551499f;
}

size_t stats_total_calc(stats_t *st) {
    #define LOG2 fastlog2
    if (!st) return 0;

    // Localized variables for compiler optimisation
    // register keywords and while uusage for speed.
    const bool entdone    =(st->entropy != 0);
    const size_t nread    = st->ntot;
    register size_t   len = st->ntot;
    register uint8_t *d   = st->data;
    uint32_t         *c   = st->counts;
    double            sum = 0;

    if(!len || !d || !st->avg_sum) return 0;   // !avg_sum --> elab() never done

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
    const double epx = 1.0 / (1U << st->base);             //st->nsybl;
    const double ex = (double)nread / (1U << st->base);    //st->nsybl;

    for (register int i = 0; i < 256; i++) {
        register double x  = (double)c[i] - ex;
        const double px = (double)c[i] / nread;
        s += (x * x) / ex;
        x = px - epx;
        k += (x * x);

        if(entdone || !c[i])
            continue;
        e -= px * LOG2(px);
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
            perror("deflateInit");
            exit(EXIT_FAILURE);
        }
    }

    // Aesthetic blankline
    if(!quiet) perr("\n");

#if 0
#define BLOCK_SIZE MAX_READ_SIZE
#else
#define BLOCK_SIZE 64
#endif

    for (unsigned k = 0; true; k) { //-- service loop start --------------- --//
        static uint64_t hash;

        // decoupling output from storage, uniforming API output
        size_t outsz;
        uint8_t *outbuf;
        #define setout(b,s) { outbuf = (uint8_t *)(b); outsz = (size_t)(s); }

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
            setout(zs.data, zs.bsize);
            ELAB(&zs);
        } else { // zdeflating already provides pass-through when requested
            PASS(P_ON);
        }
   
        if(quiet) continue;

        perr("DGB, rs(%03d)> avg: %7.3lf, ntot: %4ld, bsize: %4ld",
            ++k, rs.avg, rs.ntot, rs.bsize);
        if (J_ON) { perr(", djb2: "); print_hash(hash, 8); }
        perr("\n");
    } //-- service while end ---------------------------------------------- --//
#if 0
    SHOW(&rs); // Show read data statistics, if not inhibited
#else
    CALC(&rs);
    printstats(rs.name, rs.ntot, 256, rs.counts, 0, 1, 0);
    printstats(0, 0, 0, 0, 0, 0, 1);
#endif


    // Finalise the zlib compression process
    if (Z_ON) { // zdeflating already provides pass-through when requested
        ZDEF(Z_FINISH);
        deflateEnd(&strm);
        ELAB(&zs);
        zs.ratio = (double)zs.ntot / rs.ntot;
#if 0
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
