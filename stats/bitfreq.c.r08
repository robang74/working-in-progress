/*
 * (C) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2
 *
 * Compile: gcc -O2 -g0 -s -Wall -Wextra -Werror bitfreq.c -o bitfreq
 *
 * Usage: ./bitfreq [-s size] [-n 0..3] < input.bin > output.bin
 *
 *   -s <size>   Buffer size in bytes (positive decimal integer, default 32768)
 *   -n <0..3>   Normalization bitmask: 0=none, 1=pre-norm, 2=post-norm, 3=both (default 3)
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if CHAR_BIT != 8
#error "bitfreq requires 8-bit bytes"
#endif

#ifndef DEFAULT_BUF_SIZE
#define DEFAULT_BUF_SIZE (32u * 1024u) /* 32 KiB */
#endif

#if DEFAULT_BUF_SIZE == 0
#error "DEFAULT_BUF_SIZE must be greater than zero"
#endif

#define BYTE_VALUES 256u
#define WINDOW_SIZE 256u
#define NUM_PASSES 8u
#define MASK_FREQ_THRESHOLD 192u

#define NORM_NONE 0u
#define NORM_PRE  1u
#define NORM_POST 2u
#define NORM_BOTH 3u

static void print_usage(const char *prog, FILE *out)
{
    fprintf(out, "Usage: %s [-s size] [-n 0..3] < input.bin > output.bin\n", prog);
}

static int parse_buf_size(const char *arg, size_t *out_size)
{
    const unsigned char *p;
    char *endptr;
    uintmax_t parsed;

    if (arg == NULL || *arg == '\0')
        return -1;

    for (p = (const unsigned char *)arg; *p != '\0'; p++) {
        if (*p < '0' || *p > '9')
            return -1;
    }

    errno = 0;
    endptr = NULL;
    parsed = strtoumax(arg, &endptr, 10);
    if (errno != 0 || endptr == arg || *endptr != '\0' ||
        parsed == 0 || parsed > (uintmax_t)SIZE_MAX)
        return -1;

    *out_size = (size_t)parsed;
    return 0;
}

static int parse_norm_mode(const char *arg, unsigned int *out_mode)
{
    if (arg == NULL || arg[0] < '0' || arg[0] > '3' || arg[1] != '\0')
        return -1;

    *out_mode = (unsigned int)(arg[0] - '0');
    return 0;
}

static int read_full(int fd, unsigned char *buf, size_t len, size_t *out_read)
{
    size_t total = 0;

    while (total < len) {
        size_t request = len - total;
        ssize_t n;

        if (request > (size_t)SSIZE_MAX)
            request = (size_t)SSIZE_MAX;

        n = read(fd, buf + total, request);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0)
            break;

        total += (size_t)n;
    }

    *out_read = total;
    return 0;
}

static int write_full(int fd, const unsigned char *buf, size_t len)
{
    size_t total = 0;

    while (total < len) {
        size_t request = len - total;
        ssize_t n;

        if (request > (size_t)SSIZE_MAX)
            request = (size_t)SSIZE_MAX;

        n = write(fd, buf + total, request);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0) {
            errno = EIO;
            return -1;
        }

        total += (size_t)n;
    }

    return 0;
}

/* Find most frequent byte in buf[start .. end), tie-breaking by smallest value. */
static unsigned char find_most_frequent(const unsigned char *buf,
                                        size_t start, size_t end)
{
    size_t counts[BYTE_VALUES] = {0};
    unsigned int best_val = 0;
    size_t best_count;
    size_t i;
    unsigned int v;

    for (i = start; i < end; i++)
        counts[buf[i]]++;

    best_count = counts[0];
    for (v = 1; v < BYTE_VALUES; v++) {
        if (counts[v] > best_count) {
            best_count = counts[v];
            best_val = v;
        }
    }

    return (unsigned char)best_val;
}

/* Shift buf[start .. end) modulo 256 so that `orig` becomes `0`. */
static void shift_range_to_zero(unsigned char *buf,
                                size_t start, size_t end,
                                unsigned char orig)
{
    size_t i;

    for (i = start; i < end; i++)
        buf[i] = (unsigned char)(buf[i] - orig);
}

/* Clear `mask` on all bytes in buf[start .. end). */
static void clear_bit_mask(unsigned char *buf,
                           size_t start, size_t end,
                           unsigned char mask)
{
    unsigned char keep = (unsigned char)(~mask);

    for (size_t i = start; i < end; i++)
        buf[i] = (unsigned char)(buf[i] & keep);
}

int main(int argc, char *argv[])
{
    size_t buf_size = (size_t)DEFAULT_BUF_SIZE;
    unsigned int norm_mode = NORM_BOTH;
    size_t n_read = 0;
    size_t num_chunks = 0;
    size_t out_len = 0;
    unsigned char *buf = NULL;
    unsigned char *out_buf = NULL;
    unsigned int *masked_layers = NULL;
    size_t layer_masked_counts[NUM_PASSES] = {0};
    unsigned char pre_norm_val = 0;
    unsigned char post_norm_val = 0;
    int opt;

    static const unsigned char bit_masks[NUM_PASSES] = {
        0x80u, 0x40u, 0x20u, 0x10u, 0x08u, 0x04u, 0x02u, 0x01u
    };

    while ((opt = getopt(argc, argv, "s:n:h")) != -1) {
        switch (opt) {
        case 's':
            if (parse_buf_size(optarg, &buf_size) != 0) {
                fprintf(stderr,
                        "Invalid buffer size: '%s' (must be a positive decimal integer)\n",
                        optarg);
                return EXIT_FAILURE;
            }
            break;
        case 'n':
            if (parse_norm_mode(optarg, &norm_mode) != 0) {
                fprintf(stderr,
                        "Invalid normalization mode: '%s' (must be 0..3)\n",
                        optarg);
                return EXIT_FAILURE;
            }
            break;
        case 'h':
            print_usage(argv[0], stdout);
            return EXIT_SUCCESS;
        default:
            print_usage(argv[0], stderr);
            return EXIT_FAILURE;
        }
    }

    if (optind < argc) {
        print_usage(argv[0], stderr);
        return EXIT_FAILURE;
    }

    buf = malloc(buf_size);
    if (buf == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    if (read_full(STDIN_FILENO, buf, buf_size, &n_read) != 0) {
        perror("read");
        free(buf);
        return EXIT_FAILURE;
    }

    num_chunks = (n_read + WINDOW_SIZE - 1u) / WINDOW_SIZE;
    if (num_chunks > 0) {
        masked_layers = calloc(num_chunks, sizeof(*masked_layers));
        if (masked_layers == NULL) {
            perror("calloc");
            free(buf);
            return EXIT_FAILURE;
        }
    }

    /* Optional pre-normalization of the whole buffer before bit-plane processing. */
    if ((norm_mode & NORM_PRE) != 0u) {
        if (n_read > 0) {
            pre_norm_val = find_most_frequent(buf, 0, n_read);
            shift_range_to_zero(buf, 0, n_read, pre_norm_val);
        }

        if (fprintf(stderr, "pre-norm: %u\n", (unsigned int)pre_norm_val) < 0)
            goto stderr_error;
    }

    for (unsigned int pass = 0; pass < NUM_PASSES; pass++) {
        unsigned char mask = bit_masks[pass];
        size_t chunk_idx = 0;

        for (size_t start = 0; start < n_read; start += WINDOW_SIZE, chunk_idx++) {
            size_t end = start + WINDOW_SIZE;
            size_t win_len = 0;
            size_t ones = 0;
            size_t zeros = 0;
            unsigned int most_freq_bit = 0;
            size_t most_freq_count = 0;
            int masked_zero = 0;

            if (end > n_read)
                end = n_read;
            win_len = end - start;

            for (size_t i = start; i < end; i++) {
                if ((buf[i] & mask) != 0)
                    ones++;
                else
                    zeros++;
            }

            if (ones + zeros != win_len) {
                fprintf(stderr,
                        "Internal count mismatch on pass %u chunk %zu: ones=%zu zeros=%zu len=%zu\n",
                        pass + 1u, chunk_idx, ones, zeros, win_len);
                goto fail;
            }

            if (ones > zeros) {
                most_freq_bit = 1u;
                most_freq_count = ones;
                for (size_t i = start; i < end; i++)
                    buf[i] ^= mask;
            } else {
                most_freq_bit = 0u;
                most_freq_count = zeros;
            }

            if (most_freq_count >= MASK_FREQ_THRESHOLD) {
                clear_bit_mask(buf, start, end, mask);
                masked_layers[chunk_idx]++;
                layer_masked_counts[pass]++;
                masked_zero = 1;
            }

            if (fprintf(stderr, "%d: %d, %8zu%s\n",
                        (int)(pass + 1u), (int)most_freq_bit, most_freq_count,
                        masked_zero ? " (!)" : "") < 0)
                goto stderr_error;
        }
    }

    /* Print per-layer masked chunk summary using N: <count>/<num_chunks>. */
    for (unsigned int pass = 0; pass < NUM_PASSES; pass++) {
        if (fprintf(stderr, "%sN: %zu/%zu",
                    (pass == 0u) ? "" : ", ",
                    layer_masked_counts[pass],
                    num_chunks) < 0)
            goto stderr_error;
    }
    if (fputc('\n', stderr) == EOF)
        goto stderr_error;

    /* Assemble output by scanning chunks 0..N-1 for masked_layers == layer,
     * from layer 7 down to 0; skip chunks with 8 masked layers. */
    if (n_read > 0) {
        out_buf = malloc(n_read);
        if (out_buf == NULL) {
            perror("malloc");
            goto fail;
        }

        for (unsigned int layer = 7u; ; layer--) {
            for (size_t c = 0; c < num_chunks; c++) {
                if (masked_layers[c] == layer) {
                    size_t start = c * WINDOW_SIZE;
                    size_t end = start + WINDOW_SIZE;
                    size_t chunk_len;

                    if (end > n_read)
                        end = n_read;
                    chunk_len = end - start;

                    for (size_t i = 0; i < chunk_len; i++)
                        out_buf[out_len++] = buf[start + i];
                }
            }
            if (layer == 0u)
                break;
        }

        if ((norm_mode & NORM_POST) != 0u) {
            post_norm_val = find_most_frequent(out_buf, 0, out_len);
            shift_range_to_zero(out_buf, 0, out_len, post_norm_val);
        }
    }

    if ((norm_mode & NORM_POST) != 0u) {
        if (fprintf(stderr, "post-norm: %u (out: %zu/%zu bytes)\n",
                    (unsigned int)post_norm_val, out_len, n_read) < 0)
            goto stderr_error;
    } else {
        if (fprintf(stderr, "out: %zu/%zu bytes\n", out_len, n_read) < 0)
            goto stderr_error;
    }

    if (write_full(STDOUT_FILENO, out_buf, out_len) != 0) {
        perror("write");
        goto fail;
    }

    free(out_buf);
    free(masked_layers);
    free(buf);
    return EXIT_SUCCESS;

stderr_error:
    fprintf(stderr, "Error writing diagnostics to stderr\n");
fail:
    free(out_buf);
    free(masked_layers);
    free(buf);
    return EXIT_FAILURE;
}
