#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define MAX_PAGE_SIZE 4096

int main(int argc, char *argv[]) {
    int opt;
    size_t o_size = 0; // Offset (skip)
    size_t w_size = 0; // Window (total block)

    while ((opt = getopt(argc, argv, "o:w:")) != -1) {
        switch (opt) {
            case 'o': o_size = strtoul(optarg, NULL, 10); break;
            case 'w': w_size = strtoul(optarg, NULL, 10); break;
            default: exit(EXIT_FAILURE);
        }
    }

    // Hard limit at 4KB as per architectural design
    if (w_size > MAX_PAGE_SIZE || w_size == 0) {
        fprintf(stderr, "Error: Window size (-w) must be 1-%d bytes.\n", MAX_PAGE_SIZE);
        exit(EXIT_FAILURE);
    }

    if (o_size >= w_size) {
        fprintf(stderr, "Error: Offset (-o) must be smaller than window (-w).\n");
        exit(EXIT_FAILURE);
    }

    // Static allocation on the stack is safe here since we are < 4KB
    unsigned char buffer[MAX_PAGE_SIZE];
    size_t to_write = w_size - o_size;

    while (1) {
        size_t bytes_read = 0;

        // Fill the window completely before acting
        while (bytes_read < w_size) {
            ssize_t n = read(STDIN_FILENO, buffer + bytes_read, w_size - bytes_read);
            
            if (n == 0) return 0; // Clean EOF
            if (n < 0) {
                if (errno == EINTR) continue; // Retry on signal interrupt
                fprintf(stderr, "Read error: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            bytes_read += n;
        }

        // Write the segment after the offset
        ssize_t written = write(STDOUT_FILENO, buffer + o_size, to_write);
        
        if (written < 0) {
            fprintf(stderr, "Write error: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
