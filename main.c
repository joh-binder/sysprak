#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <string.h>

#include "shmfunctions.h"

#define GAMEKINDNAME "Bashni"
#define PORTNUMBER 1357
#define HOSTNAME "sysprak.priv.lab.nm.ifi.lmu.de"

// Hilfsnachricht zu den geforderten Kommandozeilenparametern
void printHelp(void) {
    printf("Um das Programm auszuführen, übergeben Sie bitte folgende Informationen als Kommandozeilenparameter:\n");
    printf("-g <gameid>: eine 13-stellige Game-ID\n");
    printf("-p <playerno>: Ihre gewünschte Spielernummer (1 oder 2)\n");
}

int main(int argc, char *argv[]) {

    // legt Variablen für Game-ID und Spielernummer an und liest dann Werte dafür von der Kommandozeile ein
    char gameID[14] = "";
    int playerNumber = 0;

    int input;
    while ((input = getopt(argc, argv, "g:p:")) != -1) {
        switch (input) {
            case 'g':
                strncpy(gameID, optarg, 13);
                gameID[13] = '\0';
                break;
            case 'p':
                playerNumber = atoi(optarg);
                break;
            default:
                printHelp();
                break;
        }
    }


    // legt einen Shared-Memory-Bereich mit Größe 1000 an
    int shmid = shmCreate(1000);
    void *hierIstShmemory = shmAttach(shmid);

    // Erstellung der Pipe
    int fd[2];
    char pipeBuffer[PIPE_BUF];

    if (pipe(fd) < 0) {
        perror("Fehler beim Erstellen der Pipe");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Fehler beim Forking");
        return EXIT_FAILURE;
    } else if (pid == 0) {
        // wir sind im Kindprozess -> Connector
        close(fd[1]);

        // schreibt zwei Ints in den Shared-Memory-Bereich (auf umständliche Art)
        // (wahrscheinlich würde man einfach eine neue Variable für den gecasteten Pointer anlegen)
        *(int *)hierIstShmemory = 5649754;
        *((int *)hierIstShmemory + 1) = 1122;

    } else {
        // wir sind im Elternprozess -> Thinker
        close(fd[0]);

        // liest zwei im Kindprozess geschriebene Ints aus dem Shared-Memory-Bereich
        wait(NULL);
        printf("Im Shmemory-Bereich steht: %d\n", *(int *)hierIstShmemory);
        printf("Im Shmemory-Bereich steht auch noch: %d\n", *((int *)hierIstShmemory + 1));

        if (shmDelete(shmid) > 0) return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

