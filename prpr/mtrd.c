/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, MIT license
 *
 * Usage: mtrd -nN "command or binary to execute"
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

// Struttura per passare i dati ai thread
typedef struct {
    char *cmd;
} thread_data_t;

void* spawn_and_mix(void* arg) {
    thread_data_t *data = (thread_data_t *)arg;
    
    // Apriamo un canale di lettura dal comando bash
    // Usiamo stdbuf -o0 per disabilitare il buffering nel sottoprocesso
    char final_cmd[1024];
    snprintf(final_cmd, sizeof(final_cmd),
      "nice -n19 ionice -c3 stdbuf -o0 bash -c '%s' 2>&1", data->cmd);
    
    FILE *fp = popen(final_cmd, "r");
    if (!fp) return NULL;

    int ch;
    // Leggiamo un carattere alla volta e lo spariamo su stdout
    // Il kernel scheduler deciderà quando interrompere questo thread
    while ((ch = fgetc(fp)) != EOF) {
        fputc(ch, stdout);
        fflush(stdout); // Forza l'uscita immediata del singolo byte
    }

    pclose(fp);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) return 1;

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
    return 0;
}
