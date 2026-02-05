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
#define MAX_COMP_SIZE (MAX_READ_SIZE<<1)
#define ABS(a) ((a<0)?-(a):(a))
#define MIN(a,b) ((a<b)?(a):(b))
#define MAX(a,b) ((a>b)?(a):(b))

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

unsigned printstats(const char *str, size_t nread, unsigned nsymb,
  size_t *counts, char idnt, char rset)
{
  static char entdone = 0;
  static double pavg, entropy = 0, avg = 0, n = 0;
  double x, px, k = 0, s = 0, lg2s = log2(nsymb);
  size_t i;

  if(rset) { entdone = 0; entropy = 0; avg = 0; n = 0; pavg = 0; }

  for (i = 0; i < 256; i++) {
      double ex = ((double)nread) / nsymb, epx = 1.0 / nsymb;
      x = ((double)counts[i] - ex);
      s += x * x / ex;
      px = ((double)counts[i]) / nread;
      x = px - epx;
      k += x * x;

      if(!counts[i] || entdone) continue;
      entropy -= px * log2(px);
      avg += i*counts[i];
      n++;
  }
  if(!entdone) {
    entdone = 1;
    avg = avg / nread;
    pavg = (avg/AVGV - 1)*100;
  }
  fprintf(stderr,
      "%s%s: %3ld, Eñ: %8.6lf / %4.2f = %8.6lf, X²: %8.3lf, k²: %8.5lf, avg: %8.4lf %+.4lf %%\n",
      idnt?"  ":"", str, MIN(nread,nsymb), entropy, lg2s, entropy/lg2s, s, k * nsymb, avg, pavg);

  return n;
}

void printallstats(size_t size, const char *dstr, size_t *counts, char prnt,
  double zratio)
{
    fprintf(stderr, "\n");
    if (size > 256 || prnt) {
        fprintf(stderr, "%s: %ld bytes, %.1lf Kb, %.3lf Mb", dstr,
            size, (double)size / (1<<10), (double)size / (1<<20));
        if(zratio > 0) {
          fprintf(stderr, ", zr: %lf %% (1:%.3lf)\n", zratio * 100, 1.0 / zratio);
        } else {
          long nsrun = get_nanos();
          fprintf(stderr, ", pid: %d, elab: %.1lf ms (%.1lf Kb/s)\n",
              getpid(), (double)nsrun / E6, (double)size * (E9 >> 10) / nsrun);
        }
    }
    unsigned nsymb = printstats("bytes", size, 256, counts, prnt, (zratio != 0));
    double lg2s = log2(nsymb);
    unsigned nbits = ceil(lg2s), nmax = 1 << nbits;
    if (nbits < 8)
        (void)printstats("encdg", size, nmax, counts, prnt, 0);
    if(nsymb < nmax)
        (void)printstats("symbl", size, nsymb, counts, prnt, 0);
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

static inline void *memalign(void *buf) {
  uintptr_t p = (uintptr_t)buf + 64;
  buf = (void *)((p >> 6) << 6);
  return buf;
}

size_t zdeflating(const int action, uint8_t const *zbuf, z_stream *pstrm,
    size_t hsize, size_t tsize, size_t *zcounts, bool pass)
{
    static size_t tsved = 0, zsizetot = 0;
    uint8_t *zbuffer;
    int i, ret;

    if(action != Z_NO_FLUSH && action != Z_FINISH)
        return 0;

    unsigned char *tbuffer = NULL;
    if (tsize > 0) {
        tbuffer = malloc(tsize+64);
        if (!tbuffer) {
          perror("malloc");
          exit(EXIT_FAILURE);
        }
        tbuffer = (unsigned char *)memalign(tbuffer);
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
                    writebuf(STDOUT_FILENO, (const uint8_t *)tbuffer, nz);
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
            if(pass)
                writebuf(STDOUT_FILENO, (const uint8_t *)zbuffer, zsize);
            for (i = 0; i < zsize; i++) zcounts[ zbuffer[i] ]++;
            zsizetot += zsize;
        }
    } while( (action == Z_NO_FLUSH && pstrm->avail_out == 0)
          || (action == Z_FINISH   && ret != Z_STREAM_END) );
    return zsizetot;
}

#define Z_ON (zipl >= 0)
#define perr(x...) fprintf(stderr, x)

static inline void usage(const char *name) {
    perr("\n"\
"%s read on stdin, stats on stderr, and data on stdout\n"\
"\n"\
"Usage: %s [-p] [-q] [-zN [-hN] [-tN]]\n"\
"   -q: no stats (quiet)\n"\
"   -p: data pass-through\n"\
"   -z: data compression (N:level)\n"\
"   -h: skip header (N:bytes)\n"\
"   -t: skip tail (N:bytes)\n"\
"\n", name, name);
}

int main(int argc, char *argv[]) {
    z_stream strm = {0};
    int pass = 0, zipl = -1, quiet = 0;
    size_t i, rsizetot = 0, nr = 0, zsizetot = 0, hsize = 0, tsize = 0;
    size_t nsved = 0, rcounts[256] = {0}, zcounts[256] = {0};
    unsigned char *rbuffer, rbuf[MAX_READ_SIZE+64];
    unsigned char *zbuffer, zbuf[MAX_COMP_SIZE+64];

    (void) get_nanos(); //----------------------------------------------------//

    // Memory alignment at 64 bit
    rbuffer = (unsigned char *)memalign(rbuf);
    zbuffer = (unsigned char *)memalign(zbuf);

    while (1) {
        int opt = getopt(argc, argv, "pqz:h:t:");
        //printf("opt: %d (%c), optarg: %s\n", opt, opt, optarg);
        if(opt == '?' && !optarg) {
          usage("flatz"); exit(0);
        } else if(opt == -1) break;

        switch (opt) {
            case 'p': pass = 1; break;
            case 'q': quiet = 1; break;
            case 'z': zipl = atoi(optarg); break;
            case 'h': hsize = atoi(optarg); break;
            case 't': tsize = atoi(optarg); break;
        }
    }
    // Sanitise the values
    zipl = MAX(-1, zipl);
    hsize = MAX(0, hsize);
    tsize = MAX(0, tsize);

    if (Z_ON) {
        strm.next_out = zbuf;
        strm.avail_out = MAX_COMP_SIZE;
        if (deflateInit(&strm, zipl) != Z_OK) {
            perror("deflateInit");
            exit(EXIT_FAILURE);
        }
    }

    while (1) {
        // read input stream
        nr = read(STDIN_FILENO, rbuffer, MAX_READ_SIZE);
        if (nr == 0) break;
        if (nr < 0) {
            if (errno == EINTR) continue;
            perror("read");
            exit(EXIT_FAILURE);
        }
        // write input stream
        if(pass && !Z_ON) writebuf(STDOUT_FILENO, rbuffer, nr);
        // do statistics
        for (i = 0; i < nr; i++) rcounts[ rbuffer[i] ]++; rsizetot += nr;
        // check for zip
        if (!Z_ON) continue;
        // z-compressing
        strm.avail_in = nr;
        strm.next_in = rbuffer;
        zdeflating(Z_NO_FLUSH, zbuf, &strm, hsize, tsize, zcounts, pass);
    }

    if(!quiet)
        printallstats(rsizetot, "rdata", rcounts, 1, 0.0);

    if (Z_ON) {
        zsizetot = zdeflating(Z_FINISH, zbuf, &strm, hsize, tsize, zcounts, pass);
        deflateEnd(&strm);
        if(!quiet)
        printallstats(zsizetot, "zdata", zcounts, 1, (double)zsizetot/rsizetot);
    }

    if(!quiet) { 
      fprintf(stderr, "\n");
      fflush(stderr);
    }
    fflush(stdout);

    return 0;
}
