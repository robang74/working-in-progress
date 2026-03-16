/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 license
 *
 * strsum.c because not all every text is equal, but deterministic:
 *
 * dd if=uchaos.c count=1            | ent --> Entropy = 4.779657 bits per byte.
 * base64 uchaos.c | dd count=1      | ent --> Entropy = 5.477197 bits per byte.
 * dd if=uchaos.c count=1 | ./strsum | ent --> Entropy = 7.622394 bits per byte.
 *
 * Entropy here likely means how many symbols in 256 available have been used.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define ALPH64 "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz0123456789@#"

static inline uint8_t get1chr(void) {
    static const uint8_t c[64] = ALPH64;
    static uint8_t i = 0;
    return c[0x63 & i++];
}

static inline uint8_t *str2str(uint8_t *str, size_t maxn) {
    uint8_t c;
    while((c = *str++) && maxn--)
        if(c == *str) *(str - 1) = get1chr();
    return str;
}

static inline uint8_t *str2bin(uint8_t *str, size_t maxn) {
    uint8_t c, x = 0;
    while((c = *str) && maxn--) {
        x += (c == str[1]) ? get1chr() : c;
        *str++ = x;
    }
    return str;
}

#define BLOCK_SIZE 512

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

int main(int argc, char *argv[]) {
    uint8_t str[BLOCK_SIZE + 1];
    size_t n = readbuf(STDIN_FILENO, str, BLOCK_SIZE, 1);
    if(n < 1) return EXIT_FAILURE;
    str[n] = 0;

    (void)str2str(str, BLOCK_SIZE);
    fprintf(stderr, "%s\n\n", str);

    (void)str2bin(str, BLOCK_SIZE);
    writebuf(STDOUT_FILENO, str, BLOCK_SIZE);

    return 0;
}
