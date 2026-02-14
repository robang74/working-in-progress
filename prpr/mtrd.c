/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 license
 *
 * Usage: mtrd -nN "command or binary to execute"; or -tN for timestamps also
 *
 * Compile with lib pthread: gcc mtrd.c -O3 -o mtrd -lpthread -Wall
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
#include <sys/wait.h>

#define AVGV 127.5
#define NS 1000000000L
#define MAX_READ_SIZE 16384
#define ABS(a) ((a<0)?-(a):(a))
#define MIN(a,b) ((a<b)?(a):(b))
#define MAX(a,b) ((a>b)?(a):(b))

// Struttura per passare i dati ai thread
typedef struct {
    char *cmd;
} thread_data_t;

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

void *spawn_and_mix(void* arg) {
    thread_data_t *t = (thread_data_t *)arg;
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

    char *argv[] = {"stdbuf", "-i0", "-o0", "-e0", "bash", "-c", t->cmd, NULL};

    pid_t pid;
    // posix_spawn è molto più efficiente di fork() per lanciare processi
    if (posix_spawn(&pid, "/usr/bin/stdbuf", &actions, NULL, argv, environ) != 0) {
        perror("posix_spawn");
        return NULL;
    }

    close(pipefd[1]);
    unsigned char ch;

    // Lettura a basso livello
    while (read(pipefd[0], &ch, 1) > 0) {
        // write() è atomica se la stringa è corta, ma il mixing avviene
        // tra i vari thread che chiamano read()
        if(write(STDOUT_FILENO, &ch, 1) < 0) {
            perror("write");
            break;
        }
    }

    close(pipefd[0]);
    waitpid(pid, NULL, 0);
    posix_spawn_file_actions_destroy(&actions);

    prt_nanos('<',']');
    return NULL;
}

int main(int argc, char *argv[]) {
    (void)get_nanos(); // Inizializzazione di start

    if (argc < 3) {
        fprintf(stderr, "Usage: %s -n4 \"commands\"\n", argv[0]);
        return 1;
    }

    prtnano = (argv[1][1] == 't');
    int num_threads = atoi(argv[1] + 2); // Gestisce -n4
    char *cmd = argv[2];

    prt_nanos('<','>');
    pthread_t threads[num_threads];
    thread_data_t t_data = {cmd};

    // Disabilitiamo il buffering di stdout del processo principale
    setvbuf(stdout, NULL, _IONBF, 0);

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, num_threads);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, spawn_and_mix, &t_data);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&barrier);
    prt_nanos('[',']');
    return 0;
}
