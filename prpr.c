#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define MAX_BLOCK_SIZE 512

/**
 * Reverses a buffer of a given size in-place.
 */
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
    long o_size = 0; 
    size_t w_size = 0;

    while ((opt = getopt(argc, argv, "o:w:")) != -1) {
        switch (opt) {
            case 'o': o_size = strtol(optarg, NULL, 10); break;
            case 'w': w_size = strtoul(optarg, NULL, 10); break;
            default: exit(EXIT_FAILURE);
        }
    }

    if (w_size == 0 || w_size > MAX_BLOCK_SIZE) {
        fprintf(stderr, "Error: Window (-w) must be between 1 and %d\n", MAX_BLOCK_SIZE);
        exit(EXIT_FAILURE);
    }

    size_t abs_o_size = (o_size < 0) ? -o_size : o_size;
    if (abs_o_size > w_size) {
        fprintf(stderr, "Error: Absolute offset |%ld| cannot exceed window %zu\n", o_size, w_size);
        exit(EXIT_FAILURE);
    }

    unsigned char buffer[MAX_BLOCK_SIZE];
    
    while (1) {
        size_t bytes_read = 0;

        while (bytes_read < w_size) {
            ssize_t n = read(STDIN_FILENO, buffer + bytes_read, w_size - bytes_read);
            if (n == 0) return 0; 
            if (n < 0) {
                if (errno == EINTR) continue;
                perror("read");
                exit(EXIT_FAILURE);
            }
            bytes_read += n;
        }

        // --- New Logic: Reverse vs Slice ---
        if (abs_o_size == w_size) {
            // Case: Full window reverse
            reverse_buffer(buffer, w_size);
            if (write(STDOUT_FILENO, buffer, w_size) < 0) {
                perror("write");
                exit(EXIT_FAILURE);
            }
        } else {
            // Case: Standard slicing (head or tail)
            unsigned char *output_ptr;
            size_t bytes_to_write;

            if (o_size >= 0) {
                output_ptr = buffer;
                bytes_to_write = (size_t)o_size;
            } else {
                output_ptr = buffer + (w_size - abs_o_size);
                bytes_to_write = abs_o_size;
            }

            if (bytes_to_write > 0) {
                if (write(STDOUT_FILENO, output_ptr, bytes_to_write) < 0) {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    return 0;
}
