/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, MIT
 *
 * Compile with: musl-gcc prnt16.c -o prnt16 -Wall -s -static
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#define DEFAULT_LEN  16
#define ALPH16       "0123456789abcdef"
#define PROGRAM_NAME "prnt16"
#define VERSION      "v0.0.3"

static void usage(const char *progname) {
    fprintf(stderr,
        "\n"
        "Usage: %s [OPTIONS]\n"
        "\n"
        "Synchronised output a circular sequence of hex digits (0-f) to stdout.\n"
        "\n"
        "Options:\n"
        "  -n, --len=NUMBER      Number of characters to print (default: %d)\n"
        "  -m, --method=TYPE     Output method: 'putchar' or 'write' (default: putchar)\n"
        "  -d, --delay=USECS     Delay in microseconds between characters (default: 0)\n"
        "  -v, --version         Show version information and exit\n"
        "  -h, --help            Show this help message and exit\n"
        "\n"
        "Examples:\n"
        "  %s -m write -d 5000 -n 2048     # write() + 5 ms delay + 2048 chars\n"
        "  %s -d 2000                      # putchar + 2 ms delay\n"
        "  %s --len=4096                   # 4096 chars, no delay\n"
        "\n",
        progname, DEFAULT_LEN, progname, progname, progname
    );
    fflush(stderr);
}

int main(int argc, char *argv[])
{
    size_t len          = DEFAULT_LEN;
    unsigned int delay  = 0;              // microseconds
    int use_write       = 0;              // 0 = putchar, 1 = write

    static struct option long_options[] = {
        {"method",  required_argument, 0, 'm'},
        {"len",     required_argument, 0, 'n'},
        {"delay",   required_argument, 0, 'd'},
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "m:n:d:hv", long_options, NULL)) != -1) {
        switch (c) {
            case 'm':
                if (strcmp(optarg, "putchar") == 0) {
                    use_write = 0;
                } else if (strcmp(optarg, "write") == 0) {
                    use_write = 1;
                } else {
                    fprintf(stderr, "Error: --method must be 'putchar' or 'write'\n");
                    return 1;
                }
                break;

            case 'n': {
                long val = strtol(optarg, NULL, 10);
                if (val <= 0) {
                    fprintf(stderr, "Error: length must be positive\n");
                    return 1;
                }
                len = (size_t)val;
                break;
            }

            case 'd': {
                long val = strtol(optarg, NULL, 10);
                if (val < 0) {
                    fprintf(stderr, "Error: delay cannot be negative\n");
                    return 1;
                }
                delay = (unsigned int)val;
                break;
            }

            case 'h':
                usage(argv[0]);
                return 0;

            case 'v':
                printf("%s %s\n", PROGRAM_NAME, VERSION);
                return 0;

            case '?':
                /* getopt already printed error message */
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                return 1;

            default:
                return 1;
        }
    }

    // Disable line buffering so characters appear immediately
    setvbuf(stdout, NULL, _IONBF, 0);

    const char *p = ALPH16;

    for (size_t i = 0; i < len; i++) {
        if (use_write) {
            write(1, p + (i & 0xF), 1);
            fsync(1);
        } else {
            putchar(p[i & 0xF]);
            fflush(stdout);
        }

        if (delay) {
            usleep(delay);
        }
    }

    return 0;
}
