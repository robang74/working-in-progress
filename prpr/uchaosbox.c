/*
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, GPLv2 license
 *
 * Usage: mtrd -nN "command or binary to execute"; or -tN for timestamps also
 *
 * Compile with lib pthread: gcc mtrd.c -O3 -o mtrd -lpthread -Wall
 *
 * musl-gcc mtrd.c -O3 -o mtrd -lpthread -Wall -s -static -flto -lutil -lm
 *
 */

#include <stdio.h>
#include <string.h>
#include <libgen.h>

#define PROGRAM_NAME "uchaosbox"
#define VERSION      "v0.1"

#define DECLARE_MAIN(name) int name##_main(int argc, char **argv)

DECLARE_MAIN(uchaos);
DECLARE_MAIN(prnt16);
DECLARE_MAIN(strn64);
DECLARE_MAIN(strsum);
DECLARE_MAIN(mixtrd);
DECLARE_MAIN(flatz);
DECLARE_MAIN(aprx10);
DECLARE_MAIN(prpr);
DECLARE_MAIN(prpx);

typedef struct {
    const char *name;
    int (*func)(int, char **);
    const char *desc;
} command_t;

static command_t commands[] = {
    {"uchaos",    uchaos_main,    "Descrizione per uchaos"},
    {"prnt16",    prnt16_main,    "Descrizione per prnt16"},
    {"strn64",    strn64_main,    "Descrizione per strn64"},
    {"strsum",    strsum_main,    "Descrizione per strsum"},
    {"mixtrd",    mixtrd_main,    "Descrizione per mixtrd"},
    {"flatz",     flatz_main,     "Descrizione per flatz"},
    {"aprx10",    aprx10_main,    "Descrizione per aprx10"},
    {"prpr",      prpr_main,      "Descrizione per prpr"},
    {"prpx",      prpx_main,      "Descrizione per prpx"},
    {NULL, NULL, NULL}
};

void print_usage(const char *progname) {
    printf("Usage: %s [command] [args...]\n", progname);
    printf("Built-in commands:\n ");
    for (int i = 0; commands[i].name != NULL; i++) {
        printf(" %s%s", commands[i].name, (commands[i+1].name ? "," : ""));
    }
    printf("\n\nUse '%s -v' for version or '%s [cmd] --help'"\
        " for command-specific help.\n\n", progname, progname);
}

int main(int argc, char **argv) {
    // Estraiamo il nome del file dal percorso (es. /usr/bin/prnt16 -> prnt16)
    char *cmd_name = basename(argv[0]);

    // 1. Controllo se invocato come uchaosbox direttamente
    if (strcmp(cmd_name, "uchaosbox") == 0) {
        if (argc < 2) {
            print_usage(cmd_name);
            return 1;
        }
        
        // Gestione flag uchaosbox
        if (strcmp(argv[1], "-v") == 0) {
            printf("%s %s\n", PROGRAM_NAME, VERSION);
            return 0;
        }
        if (strcmp(argv[1], "-h") == 0) {
            print_usage(cmd_name);
            return 0;
        }

        // Se passiamo il comando come primo argomento: uchaosbox prnt16 ...
        cmd_name = argv[1];
        argv++;
        argc--;
    }

    // 2. Dispatching del comando
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(cmd_name, commands[i].name) == 0) {
            return commands[i].func(argc, argv);
        }
    }

    fprintf(stderr, "%s: command not found\n", cmd_name);
    return 127;
}
