/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, MIT license
 *
 * Usage: binary stream | prpr -r [-]N -o [-]n
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define MAX_BLOCK_SIZE 512

// Macro defined with parentheses to ensure safe expansion
#define ABS(a) ((a) < 0 ? -(a) : (a))

static inline void reverse_buffer(unsigned char *buf, size_t size) {
    size_t i, j;
    for (i = 0, j = size - 1; i < j; i++, j--) {
        unsigned char temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
    }
}

static inline void xorskip_buffer(unsigned char *buf, size_t xor) {
    size_t i = 0;
    for (i = 0; i < xor; i++) {
      buf[i] ^= 0xff;
    }
}

int main(int argc, char *argv[]) {
    int opt;
    long x_arg = 0;
    long r_arg = 0;

    while ((opt = getopt(argc, argv, "x:r:")) != -1) {
        switch (opt) {
            case 'x': x_arg = strtol(optarg, NULL, 10); break;
            case 'r': r_arg = strtol(optarg, NULL, 10); break;
            default: exit(EXIT_FAILURE);
        }
    }

    size_t r_size = (size_t)ABS(r_arg);
    size_t x_abs = (size_t)ABS(x_arg);

    // 1. STOP Conditions
    if (!r_size) return 0;

    if (r_size > MAX_BLOCK_SIZE) {
        fprintf(stderr, "Error: Reading size invalid.\n");
        exit(EXIT_FAILURE);
    }

    if (x_abs > r_size) {
        fprintf(stderr, "Error: Xoring exceeds reading.\n");
        exit(EXIT_FAILURE);
    }

    unsigned char buffer[MAX_BLOCK_SIZE];

    while (1) {
        size_t bytes_read = 0, nr = 0, nw = 0;
        unsigned char *output_ptr = buffer;
        size_t bytes_to_write = 0;

        while (bytes_read < r_size) {
            nr = read(STDIN_FILENO, buffer + bytes_read, r_size - bytes_read);
            if (nr == 0) return 0;
            if (nr < 0) {
                if (errno == EINTR) continue;
                perror("read");
                exit(EXIT_FAILURE);
            }
            bytes_read += (size_t)nr;
        }

        // 2. Processing Logic
        if(x_arg * r_arg < 1)
          reverse_buffer(buffer, r_size);
        xorskip_buffer(buffer, x_abs);
        if(x_arg < 0)
          reverse_buffer(buffer, r_size);

        // 3. Single Write Execution
        if (write(STDOUT_FILENO, buffer, r_size) < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
