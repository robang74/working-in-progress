/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 license
 *
 * Usage: binary stream | flat [-p] [-q] [-zN]
 *
 * Compile with lib math: gcc flat.c -O3 -lm -o flat
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <zlib.h>

#define AVGV 125.5
#define MAX_READ_SIZE 4096
#define MAX_COMP_SIZE (MAX_READ_SIZE<<1)
#define ABS(a) ((a<0)?-(a):(a))
#define MIN(a,b) ((a<b)?(a):(b))
#define MAX(a,b) ((a>b)?(a):(b))

unsigned printstats(const char *str, size_t nread, unsigned nsymb, size_t *counts, char idnt, char rset) {
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
  fprintf(stderr, "%s%s: %4ld, Eñ: %.6lf / %.2f = %.6lf, X²: %5.3lf, k²: %3.5lf, avg: %.4lf %+.4lf %%\n",
      idnt?"  ":"", str, MIN(nread,nsymb), entropy, lg2s, entropy/lg2s, s, k * nsymb, avg, pavg);

  return n;
}

void printallstats(size_t size, const char *dstr, size_t *counts, char prnt, double zratio) {
    fprintf(stderr, "\n");
    if (size > 256 || prnt) {
        fprintf(stderr, "%s: %ld bytes, %.1lf Kb, %.3lf Mb", dstr,
            size, (double)size / (1<<10), (double)size / (1<<20));
        if(zratio > 0)
          fprintf(stderr, ", zr: %lf %% (1:%.3lf)\n", zratio * 100, 1.0/zratio);
        else
          fprintf(stderr, "\n");
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

int main(int argc, char *argv[]) {
    z_stream strm = {0};
    int opt, pass = 0, zipl = -1, quiet = 0;
    size_t i, rsizetot = 0, nr = 0, zsizetot = 0, hsize = 0, tsize = 0;
    size_t nsved = 0, rcounts[256] = {0}, zcounts[256] = {0};
    unsigned char *rbuffer, rbuf[MAX_READ_SIZE+64];
    unsigned char *zbuffer, zbuf[MAX_COMP_SIZE+64];

    // Memory alignment at 64 bit
    rbuffer = (unsigned char *)memalign(rbuf);
    zbuffer = (unsigned char *)memalign(zbuf);

    while ((opt = getopt(argc, argv, "pqz:h:t:")) != -1) {
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

    unsigned char *tbuffer = NULL;
    size_t tsved = 0;
    if (tsize > 0) {
        tbuffer = malloc(tsize+64);
        if (!tbuffer) {
          perror("malloc");
          exit(EXIT_FAILURE);
        }
        tbuffer = (unsigned char *)memalign(tbuffer);
    }

    if (zipl >= 0) {
        //if(hsize > 0)
        //   fprintf(stderr,"hsize: %ld\n", hsize);
        strm.next_out = zbuf;
        strm.avail_out = MAX_COMP_SIZE;
        if (deflateInit(&strm, zipl) != Z_OK) {
            perror("deflateInit");
            exit(EXIT_FAILURE);
        }
    }

    while (1) {
        nr = read(STDIN_FILENO, rbuffer, MAX_READ_SIZE);
        if (nr == 0) break;
        if (nr < 0) {
            if (errno == EINTR) continue;
            perror("read");
            exit(EXIT_FAILURE);
        }
        if(pass && zipl < 0) writebuf(STDOUT_FILENO, rbuffer, nr);
        for (i = 0; i < nr; i++) rcounts[ rbuffer[i] ]++;
        rsizetot += nr;
        if (zipl < 0) continue;

        strm.avail_in = nr;
        strm.next_in = rbuffer;
        do {
            zbuffer = zbuf;
            strm.next_out = zbuffer;
            strm.avail_out = MAX_COMP_SIZE;
            deflate(&strm, Z_NO_FLUSH);
            size_t zsize = MAX_COMP_SIZE - strm.avail_out;
            if(hsize && zsize > 0) {
                //fprintf(stderr,"hsize: %ld, zsize: %ld --> ", hsize, zsize);
                if(zsize >= hsize) {
                    zsize -= hsize;
                    zbuffer += hsize;
                    hsize = 0;
                } else {
                    hsize -= zsize;
                    zsize = 0;
                }
                //fprintf(stderr,"hsize: %ld, zsize: %ld\n", hsize, zsize);
            }
            if(tsize > 0 && zsize > 0) {
                if(zsize >= tsize) {
                    if(tsved > 0) {
                        writebuf(STDOUT_FILENO, (const char *)tbuffer, tsved);
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
                        writebuf(STDOUT_FILENO, (const char *)tbuffer, nz);
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
                    writebuf(STDOUT_FILENO, (const char *)zbuffer, zsize);
                for (i = 0; i < zsize; i++) zcounts[ zbuffer[i] ]++;
                zsizetot += zsize;
            }
        } while (strm.avail_out == 0);
    }

    if (zipl >= 0) {
        zbuffer = zbuf;
        int ret;
        do {
            strm.next_out = zbuffer;
            strm.avail_out = MAX_COMP_SIZE;
            ret = deflate(&strm, Z_FINISH);
            size_t zsize = MAX_COMP_SIZE - strm.avail_out;
            if(hsize && zsize > 0) {
                //fprintf(stderr,"hsize: %ld, zsize: %ld --> ", hsize, zsize);
                if(zsize >= hsize) {
                    zsize -= hsize;
                    zbuffer += hsize;
                    hsize = 0;
                } else {
                    hsize -= zsize;
                    zsize = 0;
                }
                //fprintf(stderr,"hsize: %ld, zsize: %ld\n", hsize, zsize);
            }
            if(tsize > 0 && zsize > 0) {
                if(zsize >= tsize) {
                    if(tsved > 0) {
                        writebuf(STDOUT_FILENO, (const char *)tbuffer, tsved);
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
                        writebuf(STDOUT_FILENO, (const char *)tbuffer, nz);
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
                    writebuf(STDOUT_FILENO, (const char *)zbuffer, zsize);
                for (i = 0; i < zsize; i++) zcounts[ zbuffer[i] ]++;
                zsizetot += zsize;
            }
        } while (ret != Z_STREAM_END);
        deflateEnd(&strm);
    }
    fflush(stdout);

    if(quiet) return 0;
    printallstats(rsizetot, "rdata", rcounts, 1, 0.0);
    fflush(stderr);
    if(zipl < 0) return 0;
    printallstats(zsizetot, "zdata", zcounts, 1, (double)zsizetot/rsizetot);
    fflush(stderr);

    //fprintf(stderr, "\nnsved: %ld, zipl: %d\n", nsved, zipl);
    return 0;
}
