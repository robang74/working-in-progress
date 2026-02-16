/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 license
 *
 * Compile with: gcc strsum.c -O3 --fast-math -Wall -o strsum
 * Testing with: dd if=uchaos.c count=1 | ./strsum | ent
 *
 * strsum.c because not all every text is equal, but deterministic:
 *
 * dd if=uchaos.c count=1             | ent --> Entropy = 4.779657 bits per byte.
 * dd if=uchaos.c count=1 | str2str() | ent --> Entropy = 5.408213 bits per byte.
 * base64 uchaos.c | dd count=1       | ent --> Entropy = 5.477197 bits per byte.
 * 512(base64(str2str(512(uchaos.c))))| ent --> Entropy = 5.715265 bits per byte.
 * str2s64(512(uchaos.c))             | ent --> Entropy = 5.900079 bits per byte.
 * str2bin(str2str(512(uchaos.c)))    | ent --> Entropy = 7.604514 bits per byte.
 * str2bin(512(uchaos.c))             | ent --> Entropy = 7.640274 bits per byte.
 * str2bin(str2s64(512(uchaos.c))     | ent --> Entropy = 7.660807 bits per byte.
 *
 * Entropy here likely means how many symbols in 256 available have been used.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define ALPH64 "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz0123456789@%"
#define MIN(a,b)  ( ( (a) < (b) ) ? (a) : (b) )
#define perr(x...) fprintf(stderr, x)
#define BLOCK_SIZE 512

static inline uint8_t get1chr(void) {
    static const uint8_t c[] = ALPH64;
    static uint8_t i = 0;
    return c[0x3F & i++];
}

static inline size_t str2str(uint8_t *str, size_t maxn) {
    uint8_t c, dup[BLOCK_SIZE + 1];
    size_t i;
    maxn = MIN(maxn, BLOCK_SIZE);
    for (i = 0; (c = str[i]) && i < maxn; i++)
        dup[i] = (c == str[i+1]) ? get1chr() : c;
    dup[i] = 0;
    memcpy(str, dup, maxn);
    return i;
}

static inline size_t str2bin(uint8_t *str, size_t maxn) {
    uint8_t acc = 0;
    size_t i;
    for (i = 0; i < maxn && str[i]; i++) {
        acc += str[i];
        str[i] = acc;
    }
    str[i] = 0;
    return i;
}

static inline size_t str2s64(uint8_t *str, size_t maxn) {
    static const uint8_t c[] = ALPH64;
    size_t i, n = str2bin(str, maxn);
    for (i = 0; i < n; i++) {
        str[i] = c[str[i] & 0x3F];
    }
    str[i] = 0;
    return i;   
}

/* ************************************************************************** */

static inline ssize_t readbuf(int fd, uint8_t *buffer, size_t size, bool intr) {
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

/* ************************************************************************** */

int main(int argc, char *argv[]) {
    uint8_t str[BLOCK_SIZE + 1];
    size_t n = readbuf(STDIN_FILENO, str, BLOCK_SIZE, 1);
    if(n < 1) return EXIT_FAILURE;
    str[n] = 0;

    n = str2s64(str, n);
    for (size_t i = 0; i < n; i += 64) {
        perr("\n");
        writebuf(STDERR_FILENO, &str[i], 64);
    }
    perr("\n\n");
#ifdef _OUTPUT_TEXT_ONLY
    writebuf(STDOUT_FILENO, str, n);
    return 0;
#else
    n = str2bin(str, n);
    writebuf(STDOUT_FILENO, str, n);
#endif
    return 0;
}
