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
#include <stdint.h>
#include <getopt.h>

#ifdef _USE_STDBUF
#else
#include <pty.h>        // forkpty() in musl only
#include <utmp.h> 
#endif

#define PROGRAM_NAME "mixtrd"
#define VERSION      "v0.5.4"

#define AVGV 127.5
#define E3 1000L
#define E6 1000000L
#define E9 1000000000L
#define MAX_READ_SIZE 16384
#define ABS(a) ((a<0)?-(a):(a))
#define MIN(a,b) ((a<b)?(a):(b))
#define MAX(a,b) ((a>b)?(a):(b))

// Funzione per ottenere il tempo in nanosecondi
static uint64_t get_nanos(void) {
    static uint64_t start = 0;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (!start) {
        start = (uint64_t)ts.tv_sec * E9 + ts.tv_nsec;
        return start;
    }
    return ((uint64_t)ts.tv_sec * E9 + ts.tv_nsec) - start;
}

#define NS E9
#define BFLN 32
unsigned char prtnano = 0;
static inline void prt_nanos(unsigned char a, unsigned char b) {
    static char first = 1;
    unsigned char buf[BFLN], len;
    uint64_t nanos;

    if(!prtnano) return;

    nanos = get_nanos();
    if(first) {
        first = 0;
        len = snprintf((char *)buf, BFLN, "%cs.%09ld%c\n", a, nanos % NS, b);
    } else {
        len = snprintf((char *)buf, BFLN, "\n%c%ld.%09ld%c\n", a, nanos / NS, nanos % NS, b);
    }
    if(len > BFLN-1) len = BFLN-1;
    buf[BFLN-1] = 0;

#if 0
    fprintf(stdout, "%s", buf);
    fflush(stdout);
#else
    write(fileno(stdout), buf, len);
#endif

    return;
}

extern char **environ;
static pthread_barrier_t barrier;

void *spawn_and_mix(void *arg) {
    char *cmd = (char *)arg;
    int pipefd[2] = { -1, -1 };

    prt_nanos('[','>');

#ifdef _USE_STDBUF
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
#else
    pid_t pid = forkpty(&pipefd[0], NULL, NULL, NULL);
    if (pid < 0) {
        perror("forkpty");
        return NULL;
    }

    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        perror("execl /bin/sh -c");
        _exit(127);
    }
#endif

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
#ifdef _USE_STDBUF
#else
        if (errno != EIO)
#endif
        perror("read from pipe");
    }

    close(pipefd[0]);
    waitpid(pid, NULL, 0);
#ifdef _USE_STDBUF
    posix_spawn_file_actions_destroy(&actions);
#endif

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
#ifdef _USE_STDBUF
        "\n"
        "Note: compiled w/ _USE_STDBUF and it requires /usr/bin/stdbuf.\n"
#endif
        "\n",
        progname, progname, progname, progname
    );
    fflush(stderr);
}

int main(int argc, char *argv[]) {
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

/* ************************************************************************** */
    if (fork() != 0) _exit(0);
    setsid();
    if (fork() != 0) _exit(0);
/* ************************************************************************** */

    const char *cmd = argv[optind];
    
    (void)get_nanos(); // Inizializzazione di start

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
