/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 license
 *
 * Usage: mtrd -nN "command or binary to execute"; or -tN for timestamps also
 *
 * Compile with lib pthread: gcc mtrd.c -O3 -o mtrd -lpthread -Wall
 *
 * Static ELF binary: musl-gcc mtrd.c -O3 -o mtrd -lpthread -Wall -s -static
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <spawn.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <getopt.h>

#define PROGRAM_NAME "mtrd"
#define VERSION      "v0.5.1"

#define AVGV 127.5
#define NS 1000000000L
#define MAX_READ_SIZE 16384
#define ABS(a) ((a<0)?-(a):(a))
#define MIN(a,b) ((a<b)?(a):(b))
#define MAX(a,b) ((a>b)?(a):(b))

// Funzione per ottenere il tempo in nanosecondi
long get_nanos() {
    static long start = 0;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (!start) {
      start = (long)ts.tv_sec * 1000000000L + ts.tv_nsec;
      return start;
    }
    return ((long)ts.tv_sec * 1000000000L + ts.tv_nsec) - start;
}

unsigned char prtnano = 0;
static inline void prt_nanos(unsigned char a, unsigned char b) {
    const int BFLN = 32;
    static char first = 1;
    unsigned char buf[BFLN];
    long nanos;

    if(!prtnano) return;

    nanos = get_nanos();
    if(first) {
        first = 0;
        snprintf((char *)buf, BFLN, "%cs.%09ld%c\n", a, nanos % NS, b);
    } else {
        snprintf((char *)buf, BFLN, "\n%c%ld.%09ld%c\n", a, nanos / NS, nanos % NS, b);
    }

    buf[BFLN-1] = 0;
    fprintf(stdout, "%s", buf);
    fflush(stdout);

    return;
}

extern char **environ;
static pthread_barrier_t barrier;

void *spawn_and_mix(void *arg) {
    char *cmd = (char *)arg;
    int pipefd[2];

    prt_nanos('[','>');
    if(pipe(pipefd) < 0) {
        perror("pipe");
        return NULL;
    }

    // Prepariamo l'azione per duplicare i descrittori di file nel figlio
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDERR_FILENO);
    posix_spawn_file_actions_addclose(&actions, pipefd[0]);

    char *argv[] = {"stdbuf", "-i0", "-o0", "-e0", "bash", "-c", cmd, NULL};

    pid_t pid;
    // posix_spawn è molto più efficiente di fork() per lanciare processi
    int ret = posix_spawn(&pid, "/usr/bin/stdbuf", &actions, NULL, argv, environ);
    if (ret != 0) {
        errno = ret;
        perror("posix_spawn");
        posix_spawn_file_actions_destroy(&actions);
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }

    close(pipefd[1]);
    pthread_barrier_wait(&barrier); // The aim is to sync the output producers

    // Lettura a basso livello
    ssize_t n;
    unsigned char ch;
    while ((n = read(pipefd[0], &ch, 1)) > 0) {
        // write() è atomica se la stringa è corta, ma il mixing avviene
        // tra i vari thread che chiamano read()
        if(write(STDOUT_FILENO, &ch, 1) < 0) {
            perror("write");
            break;
        }
    }
    if (n < 0) {
        perror("read from pipe");
    }

    close(pipefd[0]);
    waitpid(pid, NULL, 0);
    posix_spawn_file_actions_destroy(&actions);

    prt_nanos('<',']');
    return NULL;
}

static void print_usage(const char *progname) {
    fprintf(stderr,
        "\n"
        "Usage: %s [OPTIONS] COMMAND\n"
        "\n"
        "Run N parallel instances of the same COMMAND and multiplex their\n"
        "stdout + stderr output into a single stream.\n"
        "\n"
        "Options:\n"
        "  -n N     Run N parallel instances (required, N ≥ 1)\n"
        "  -t       Print relative timestamps [s.nnnnnnnnn] around each instance\n"
        "  -h       Show this help message and exit\n"
        "\n"
        "Examples:\n"
        "  %s -n6 \"echo $RANDOM; sleep 1\"\n"
        "  %s -t4 \"curl -s http://example.com\"\n"
        "  %s -h\n"
        "\n"
        "Note: requires /usr/bin/stdbuf to disable buffering.\n"
        "\n",
        progname, progname, progname, progname
    );
    fflush(stderr);
}

int main(int argc, char *argv[]) {
    (void)get_nanos(); // Inizializzazione di start

    int num_threads = 0;
    int c;

    while ((c = getopt(argc, argv, "n:th")) != -1) {
        switch (c) {
            case 'n':
                num_threads = atoi(optarg);
                if (num_threads < 1) {
                    fprintf(stderr, "Error: number of instances must be >= 1\n");
                    return 1;
                }
                break;
            case 't':
                prtnano = 1;
                break;
            case 'h':
                print_usage(PROGRAM_NAME " " VERSION);
                return 0;
            case 'v':
                printf("%s %s\n", PROGRAM_NAME, VERSION);
                return 0;
            default:
                return 1;
        }
    }

    if (optind >= argc || num_threads == 0) {
        fprintf(stderr, "Error: missing COMMAND or -n option\n");
        fprintf(stderr, "Try '%s -h' for usage.\n", argv[0]);
        return 1;
    }

    const char *cmd = argv[optind];

    // Make parent's own stdout unbuffered (helps when we mix writes)
    setvbuf(stdout, NULL, _IONBF, 0);

    prt_nanos('<','>');
    pthread_t threads[num_threads];

    // Disabilitiamo il buffering di stdout del processo principale
    setvbuf(stdout, NULL, _IONBF, 0);

    pthread_barrier_init(&barrier, NULL, num_threads);

    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, spawn_and_mix, (void *)cmd) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&barrier);
    prt_nanos('[',']');
    return 0;
}
