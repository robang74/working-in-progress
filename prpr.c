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

int main(int argc, char *argv[]) {
    int opt;
    long o_arg = 0; 
    long w_arg = 0;

    while ((opt = getopt(argc, argv, "o:w:")) != -1) {
        switch (opt) {
            case 'o': o_arg = strtol(optarg, NULL, 10); break;
            case 'w': w_arg = strtol(optarg, NULL, 10); break;
            default: exit(EXIT_FAILURE);
        }
    }

    size_t w_size = (size_t)ABS(w_arg);
    size_t o_abs = (size_t)ABS(o_arg);

    // 1. Symmetric STOP Condition
    if (w_size == o_abs) return 0;

    if (w_size == 0 || w_size > MAX_BLOCK_SIZE) {
        fprintf(stderr, "Error: Window size invalid.\n");
        exit(EXIT_FAILURE);
    }

    if (o_abs > w_size) {
        fprintf(stderr, "Error: Offset exceeds window.\n");
        exit(EXIT_FAILURE);
    }

    unsigned char buffer[MAX_BLOCK_SIZE];

    while (1) {
        size_t bytes_read = 0;
        while (bytes_read < w_size) {
            ssize_t nr = read(STDIN_FILENO, buffer + bytes_read, w_size - bytes_read);
            if (nr == 0) return 0;
            if (nr < 0) {
                if (errno == EINTR) continue;
                perror("read");
                exit(EXIT_FAILURE);
            }
            bytes_read += (size_t)nr;
        }

        unsigned char *output_ptr = buffer;
        size_t bytes_to_write = 0;

        // 2. Processing Logic
        if (w_arg < 0) {
            /* * Centered Mode (Removal): Overwrite the middle portion.
             * Logic: Keep first 'n' bytes, skip 'o_abs' bytes, keep the rest.
             */
            size_t n_head = (w_size - o_abs) / 2;
            size_t n_tail = w_size - (n_head + o_abs);
            
            if (n_tail > 0) {
                // Move tail forward to overwrite the 'o_abs' hole
                // memmove is used because the source and dest might overlap
                memmove(buffer + n_head, buffer + n_head + o_abs, n_tail);
            }
            
            output_ptr = buffer;
            bytes_to_write = n_head + n_tail;
        } 
        else {
            /* Standard Slice Mode (Head/Tail) */
            output_ptr = (o_arg >= 0) ? buffer : (buffer + (w_size - o_abs));
            bytes_to_write = o_abs;
        }

        // 3. Single Write Execution
        if (bytes_to_write > 0) {
            ssize_t nw = write(STDOUT_FILENO, output_ptr, bytes_to_write);
            if (nw < 0) {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }
    }
    return 0;
}
