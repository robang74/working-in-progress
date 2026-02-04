/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, MIT license
 *
 * Usage: binary stream | flat
 *
 * Compile with lib math: gcc dstr.c -O3 -lm -o flat 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#define MAX_READ_SIZE 4096

unsigned printstats(const char *str, size_t nread, unsigned nsymb, size_t *counts) {
  size_t i;
  double x, px, k = 0, s = 0, entropy = 0, n = 0, lg2s = log2(nsymb);

  for (i = 0; i < 256; i++) {
      double ex = ((double)nread) / nsymb, epx = 1.0 / nsymb;
      x = ((double)counts[i] - ex);
      s += x * x  / ex;
      px = ((double)counts[i]) / nread;
      x = px - epx;
      k += x * x;
      if(counts[i]) n++; else continue;
      entropy -= px * log2(px);
  }
  printf("%s: %4ld, Eñ: %.6lf / %.2f = %.6lf, X²: %5.3lf, k²: %3.5lf\n",
      str, nread, entropy, lg2s, entropy/lg2s, s, k * nsymb);

  return n;
}

int main(int argc, char *argv[]) {
    size_t bytes_read = 0, nr = 0, i, counts[256];  
    unsigned char buffer[MAX_READ_SIZE];
    
    for (i = 0; i < 256; i++)
        counts[i] = 0;

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

  unsigned n = printstats("bytes", bytes_read, 256, counts);

  double lg2s = log2(n);
  unsigned nbits = ceil(lg2s), nmax = 1 << nbits;

  if (nbits < 8) {
      (void)printstats("encdg", nmax, nmax, counts);
/*
      for (entropy = 0, s = 0, k = 0, i = 0; i < 256; i++) {
          double ex = ((double)bytes_read) / nmax, epx = 1.0 / nmax;
          x = ((double)counts[i] - ex);
          s += x * x  / ex;
          px = ((double)counts[i]) / nmax;
          x = px - epx;
          k += x * x;
          if(!counts[i]) continue;
          entropy -= px * log2(px);
      }
      printf("encdg: %4u, Eñ: %.6lf / %u.00 = %.6lf, X²: %5.3lf, k²: %2.6lf\n",
          (unsigned) nmax, entropy, nbits, entropy/nbits, s, k * n);
*/
  }
/*
  if(n < nmax) {
      for (entropy = 0, s = 0, k = 0, i = 0; i < 256; i++) {
          double ex = ((double)bytes_read) / n, epx = 1.0 / n;
          x = ((double)counts[i] - ex);
          s += x * x  / ex;
          px = ((double)counts[i]) / n;
          x = px - epx;
          k += x * x; // TODO
          if(!counts[i]) continue;
          entropy -= px * log2(px);
      }
      entropy = (entropy * n)/nmax;
      printf("symbl: %4u, Eñ: %.6lf / %.2f = %.6lf, X²: %5.3lf, k²: %2.6lf\n",
          (unsigned) n, entropy, lg2s, entropy/lg2s, s, k * n);
  }
*/
    return 0;
}
