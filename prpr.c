#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define MAX_PAGE_SIZE 4096

int main(int argc, char *argv[]) {
    int opt;
    long o_val = 0;   // Signed offset/slice value
    size_t w_size = 0; // Window (stride) size

    while ((opt = getopt(argc, argv, "o:w:")) != -1) {
        switch (opt) {
            case 'o': o_val = strtol(optarg, NULL, 10); break;
            case 'w': w_size = strtoul(optarg, NULL, 10); break;
            default: exit(EXIT_FAILURE);
        }
    }

    // Constraints check
    if (w_size == 0 || w_size > MAX_PAGE_SIZE) {
        fprintf(stderr, "Error: Window (-w) must be between 1 and %d\n", MAX_PAGE_SIZE);
        exit(EXIT_FAILURE);
    }

    size_t abs_o = (o_val < 0) ? -o_val : o_val;
    if (abs_o > w_size) {
        fprintf(stderr, "Error: Absolute offset |%ld| cannot exceed window %zu\n", o_val, w_size);
        exit(EXIT_FAILURE);
    }

    unsigned char buffer[MAX_PAGE_SIZE];
    unsigned char *output_ptr;
    size_t bytes_to_write;

    // Logic: 
    // If o is 3: print first 3 (buffer, len 3)
    // If o is -1: print last 1 (buffer + (4-1), len 1)
    if (o_val >= 0) {
        output_ptr = buffer;
        bytes_to_write = (size_t)o_val;
    } else {
        output_ptr = buffer + (w_size - abs_o);
        bytes_to_write = abs_o;
    }

    while (1) {
        size_t bytes_read = 0;

        // Fill the window
        while (bytes_read < w_size) {
            ssize_t n = read(STDIN_FILENO, buffer + bytes_read, w_size - bytes_read);
            if (n == 0) return 0; // End of stream
            if (n < 0) {
                if (errno == EINTR) continue;
                perror("read");
                exit(EXIT_FAILURE);
            }
            bytes_read += n;
        }

        // Write the slice
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
