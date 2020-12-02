#include <sys/shm.h>
#include <stdio.h>

#include "shmfunctions.h"

int shmCreate(int shmDataSize) {
    int shmid = shmget(IPC_PRIVATE, shmDataSize, IPC_CREAT | IPC_EXCL | SHM_R | SHM_W);
    if (shmid < 0) {
        perror("Fehler bei Shared-Memory-Erstellung");
    }
    printf("(angelegt) Shared-Memory-ID: %d\n", shmid); // nur zur Kontrolle, kann weg
    return shmid;
}

void *shmAttach(int shmid) {
    void *shmPointer;
    shmPointer = shmat(shmid, NULL, 0);
    if (shmPointer == (void *) -1) {
        perror("Fehler beim Anbinden von Shared Memory");
    }
    return shmPointer;
}

int shmDetach(void *shmpointer) {
    int detachSuccess = shmdt(shmpointer);
    if (detachSuccess == -1) {
        perror("Fehler beim Abbinden von Shared Memory");
    }
    return detachSuccess;
}

int shmDelete(int shmid) {
    int deleteSuccess = shmctl(shmid, IPC_RMID, 0);
    if (deleteSuccess == -1) {
        perror("Fehler beim Löschen von Shared Memory");
    }
    return deleteSuccess;
}