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

int main(int argc, char *argv[]) {
    size_t bytes_read = 0, nr = 0, i;  
    unsigned char buffer[MAX_READ_SIZE];
    size_t counts[256];
    
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

  unsigned int n = 0;
  double x, k, s, entropy;

  for (entropy = 0, s = 0, k = 0, i = 0; i < 256; i++) {
      if(counts[i]) n++;
      double ex = ((double)bytes_read) / 256.0;
      x = ((double)counts[i] - ex);
      s += x * x  / ex;
      if(!counts[i]) continue;
      x = ((double)counts[i]) / bytes_read;
      entropy -= x * log2(x);
      x -= ex;
      x *= x;
      k += x; // TODO
  }
  printf("bytes: %4ld, E: %12lf, X²: %12lf\n", bytes_read, entropy, s);

  for (entropy = 0, s = 0, k = 0, i = 0; i < 256; i++) {
      if(!counts[i]) continue;
      double ex = ((double)bytes_read) / n;
      x = ((double)counts[i] - ex);
      s += x * x  / ex;
      x = ((double)counts[i]) / n;
      entropy -= x * log2(x);
  }
  printf("symbl: %4u, E: %12lf, X²: %12lf\n", n, entropy, s);
/* 
   for (i = 0; i < 256; i++) {
      if(counts[i])
        printf("%3ld: %6ld\n", i, counts[i]);
   }
*/
}
