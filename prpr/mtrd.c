/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, MIT license
 *
 * Usage: mtrd -nN "command or binary to execute"
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

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
      return 0;
    }
    return ((long)ts.tv_sec * 1000000000L + ts.tv_nsec) - start;
}

#define NS 1000000000L
void prt_nanos(unsigned char a, unsigned char b) {
    long nanos = get_nanos();
    fprintf(stdout, "%c%ld.%09ld%c\n", a, nanos / NS, nanos % NS, b);
    fflush(stdout);
}


void* spawn_and_mix(void* arg) {
    thread_data_t *data = (thread_data_t *)arg;
    char final_cmd[2048];

    // Usiamo stdbuf su tutto il comando e uniamo gli stream.
    // L'aggiunta di 'stdbuf -i0 -o0 -e0' è fondamentale.
    snprintf(final_cmd, sizeof(final_cmd), 
             "stdbuf -i0 -o0 -e0 bash -c '%s' 2>&1", data->cmd);
    // Apriamo un canale di lettura dal comando bash
    FILE *fp = popen(final_cmd, "r");
    if (!fp) return NULL;

    prt_nanos('[','>');

    int ch;
    // Leggiamo un carattere alla volta e lo spariamo su stdout
    // Il kernel scheduler deciderà quando interrompere questo thread
    while ((ch = fgetc(fp)) != EOF) {
        // Formato: [NANOSECONDI] BYTE
        // Usiamo un printf atomico per evitare che il timestamp stesso venga spezzato
        putchar((unsigned char)ch);
        fflush(stdout); // Forza l'uscita immediata del singolo byte
    }
    
    prt_nanos('<',']');

    pclose(fp);
    return NULL;
}

int main(int argc, char *argv[]) {
    (void)get_nanos(); // Inizializzazione di start

    if (argc < 3) {
        fprintf(stderr, "Uso: %s -n4 \"comando\"\n", argv[0]);
        return 1;
    }

    int num_threads = atoi(argv[1] + 2); // Gestisce -n4
    char *cmd = argv[2];

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
