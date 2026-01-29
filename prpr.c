#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define MAX_BLOCK_SIZE 512

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

    size_t w_size = (size_t)(w_arg < 0 ? -w_arg : w_arg);
    size_t o_abs = (size_t)(o_arg < 0 ? -o_arg : o_arg);

    // 1. STOP Condition: If absolute values are equal and w was negative
    if (w_arg < 0 && w_size == o_abs) {
        return 0; 
    }

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
            ssize_t n = read(STDIN_FILENO, buffer + bytes_read, w_size - bytes_read);
            if (n == 0) return 0;
            if (n < 0) {
                if (errno == EINTR) continue;
                perror("read");
                exit(EXIT_FAILURE);
            }
            bytes_read += n;
        }

        // 2. Processing Logic
        if (o_abs == w_size) {
            // Reversal logic (triggered by equal abs values, but not the STOP case)
            reverse_buffer(buffer, w_size);
            write(STDOUT_FILENO, buffer, w_size);
        } 
        else if (w_arg < 0) {
            // Centered Mode: Logic (w-o)/2
            // Note: This implementation assumes we print o_abs chars starting at 'start'
            size_t start = (w_size - o_abs) / 2;
            write(STDOUT_FILENO, buffer + start, o_abs);
        } 
        else {
            // Standard Slice Mode (Head/Tail)
            unsigned char *ptr = (o_arg >= 0) ? buffer : (buffer + (w_size - o_abs));
            write(STDOUT_FILENO, ptr, o_abs);
        }
    }
    return 0;
}
