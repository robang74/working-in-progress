/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, MIT license
 *
 * Usage: binary stream | flat
 *
 * Compile with lib math: gcc dstr.c -O3 -lm -o flat 
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#define AVG 125.5
#define MAX_READ_SIZE 4096
#define MIN(a,b) ((a<b)?(a):(b))

unsigned printstats(const char *str, size_t nread, unsigned nsymb, size_t *counts) {
  static char entdone = 0;
  static double pavg, entropy = 0, avg = 0, n = 0;
  double x, px, k = 0, s = 0, lg2s = log2(nsymb);
  size_t i;

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
    pavg = (avg/AVG - 1)*100;
  }
  printf("%s: %4ld, Eñ: %.6lf / %.2f = %.6lf, X²: %5.3lf, k²: %3.5lf, avg: %.4lf %+.4lf %%\n",
      str, MIN(nread,nsymb), entropy, lg2s, entropy/lg2s, s, k * nsymb, avg, pavg);

  return n;
}

int main(int argc, char *argv[]) {
    size_t bytes_read = 0, nr = 0, i, counts[256] = {0};
    unsigned char *buffer, buf[MAX_READ_SIZE+64];
  
    // Memory alignment at 64 bit
    uintptr_t p = (uintptr_t)buf + 64;
    buffer = (unsigned char *)((p >> 6) << 6);

    while (1) {
        nr = read(STDIN_FILENO, buffer, MAX_READ_SIZE);
        if (nr == 0) break;
        if (nr < 0) {
            if (errno == EINTR) continue;
            perror("read");
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < nr; i++)
            counts[ buffer[i] ]++;
        bytes_read += nr;
    }

    if (bytes_read > 256)
        printf("size : %ld bytes, %.1lf Kb, %.3lf Mb\n",
            bytes_read, (double)bytes_read/(1<<10), (double)bytes_read/(1<<20));

    unsigned nsymb = printstats("bytes", bytes_read, 256, counts);
    double lg2s = log2(nsymb);
    unsigned nbits = ceil(lg2s), nmax = 1 << nbits;
     
    if (nbits < 8)
        (void)printstats("encdg", bytes_read, nmax, counts);

    if(nsymb < nmax)
        (void)printstats("symbl", bytes_read, nsymb, counts);

    return 0;
}
