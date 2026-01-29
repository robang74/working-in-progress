#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * Prepper: Discards 'skip' bytes and prints the remaining 'read' bytes
 * from a total block size of 'read'.
 */

int main(int argc, char *argv[]) {
    int opt;
    size_t skip = 0;
    size_t total_block = 0;

    // 1. Parsing options -s and -r
    while ((opt = getopt(argc, argv, "s:r:")) != -1) {
        switch (opt) {
            case 's':
                skip = strtoul(optarg, NULL, 10);
                break;
            case 'r':
                total_block = strtoul(optarg, NULL, 10);
                break;
            default:
                fprintf(stderr, "Usage: %s -s <skip_bytes> -r <total_block_size>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Validation: ensure total_block is larger than skip
    if (total_block <= skip || total_block == 0) {
        fprintf(stderr, "Error: -r (total) must be greater than -s (skip) and non-zero.\n");
        exit(EXIT_FAILURE);
    }

    size_t to_print = total_block - skip;
    unsigned char *buffer = malloc(total_block);
    if (!buffer) {
        perror("Buffer allocation failed");
        return 1;
    }

    // 2. Processing the stream
    // fread will wait for the input stream if it's slow (like a pipe)
    while (1) {
        size_t bytes_read = 0;
        
        // Ensure we read the FULL block size before processing
        while (bytes_read < total_block) {
            size_t n = fread(buffer + bytes_read, 1, total_block - bytes_read, stdin);
            if (n == 0) {
                // End of stream or error
                free(buffer);
                return 0; 
            }
            bytes_read += n;
        }

        // 3. Output the portion after the 'skip' offset
        fwrite(buffer + skip, 1, to_print, stdout);
        
        // Flush to ensure data moves through the pipe immediately
        fflush(stdout);
    }

    free(buffer);
    return 0;
}
