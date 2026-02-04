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
#include <string.h>
#include <math.h>

#define MAX_READ_SIZE 4096

void main(void) {
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

  double x, s = 0;
  unsigned int n = 0;
  for (i = 0; i < 256; i++) {
      if(counts[i]) n++;
      x = (double)counts[i] - (double)bytes_read / 256;
      s += x * x;
      //printf("i: %3ld, c: %3ld / %ld, x: %lf, s: %lf\n", i, counts[i], bytes_read, x, s);
  }
  printf("bytes: %4ld, E: %16lf, E²: %16lf\n", bytes_read, sqrt(s), s);
  for (i = 0; i < 256; i++) {
      x = (double)counts[i] - (double)bytes_read / n;
      s += x * x;
      //printf("i: %3ld, c: %3ld / %ld, x: %lf, s: %lf\n", i, counts[i], bytes_read, x, s);
  }
  printf("symbl: %4u, E: %16lf, E²: %16lf\n", n, sqrt(s), s);
/* 
   for (i = 0; i < 256; i++) {
      if(counts[i])
        printf("%3ld: %6ld\n", i, counts[i]);
   }
*/
}
