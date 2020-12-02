#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>

#include "shmfunctions.h"

int main(int argc, char *argv[]) {

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

