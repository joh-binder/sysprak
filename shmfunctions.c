#include <sys/shm.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "shmfunctions.h"

// erzeugt ein neues struct gameInfo und initialisiert es mit Standardwerten
struct gameInfo createGameInfoStruct(void) {
    struct gameInfo ret;
    strcpy(ret.gameName, "");
    ret.playerNumber = 0;
    ret.numberOfPlayers = 0;
    ret.pidConnector = 0;
    ret.pidThinker = 0;
    return ret;
}

// erzeugt ein neues struct gameInfo und initialisiert es mit übergebenen Werten für playerNumber, playerName und readyOrNot
struct playerInfo createPlayerInfoStruct(int pN, char *name, int ready, int shmid) {
    struct playerInfo ret;
    ret.playerNumber = pN;
    strcpy(ret.playerName, name);
    ret.readyOrNot = ready;
    ret.nextPlayerPointer = NULL;
    ret.ownShmid = shmid;
    return ret;
}

/* Durchsucht eine Liste von struct playerInfos nach einer gegebenen Spielernummer, gibt einen
 * Pointer auf das struct des entsprechenden Spielers zurück, falls vorhanden; ansonsten Nullpointer;
 * Eingabeparameter sind ein Pointer auf ein struct playerInfo, ab dem gesucht werden soll (i.d.R. das erste Struct der
 * Liste) sowie die gewünschte Spielernummer
 */
struct playerInfo *getPlayerFromNumber(struct playerInfo *pCurrentPlayer, int targetNumber) {
    if (pCurrentPlayer->playerNumber == targetNumber) {
        return pCurrentPlayer;
    } else {
        if (pCurrentPlayer->nextPlayerPointer == NULL) {
            return NULL;
        } else {
            return getPlayerFromNumber(pCurrentPlayer->nextPlayerPointer, targetNumber);
        }
    }
}


// Erzeugt ein Shared-Memory-Segment einer gegebenen Größe und gibt dessen ID zurück, oder -1 im Fehlerfall.
int shmCreate(int shmDataSize) {
    int shmid = shmget(IPC_PRIVATE, shmDataSize, IPC_CREAT | IPC_EXCL | SHM_R | SHM_W);
    if (shmid < 0) {
        perror("Fehler bei Shared-Memory-Erstellung");
    }
    printf("(angelegt) Shared-Memory-ID: %d\n", shmid); // nur zur Kontrolle, kann weg
    return shmid;
}

/* Bindet das Shared-Memory-Segment einer gegebenen ID an den Adressraum an und gibt einen Pointer auf die
 * Anfangsadresse zurück, oder (void *) -1 im Fehlerfall. */
void *shmAttach(int shmid) {
    void *shmPointer;
    shmPointer = shmat(shmid, NULL, 0);
    if (shmPointer == (void *) -1) {
        perror("Fehler beim Anbinden von Shared Memory");
    }
    return shmPointer;
}

/* Bindet das Shared-Memory-Segment zu einem gegebenen Pointer wieder vom Adressraum ab.
 * Gibt 0 im Erfolgsfall und -1 im Fehlerfall zurück. */
int shmDetach(void *shmpointer) {
    int detachSuccess = shmdt(shmpointer);
    if (detachSuccess == -1) {
        perror("Fehler beim Abbinden von Shared Memory");
    }
    return detachSuccess;
}

/* Löscht das Shared-Memory-Segment zu einer gegebenen ID.
 * Gibt 0 im Erfolgsfall und -1 im Fehlerfall zurück. */
int shmDelete(int shmid) {
    int deleteSuccess = shmctl(shmid, IPC_RMID, 0);
    if (deleteSuccess == -1) {
        perror("Fehler beim Löschen von Shared Memory");
    }
    return deleteSuccess;
}

// entfernt alle für einzelnen Spieler angelegten Shared-Memory-Bereiche; gibt 0 bei Erfolg und -1 im Fehlerfall zurück
int deletePlayerShm(struct playerInfo *currentPlayer) {
    if (currentPlayer->nextPlayerPointer != NULL) {
        int deleteSucess = deletePlayerShm(currentPlayer->nextPlayerPointer) + shmDelete(currentPlayer->ownShmid);
        if (deleteSucess != 0) {
            return -1;
        } else {
            return 0;
        }
    } else {
        return shmDelete(currentPlayer->ownShmid);
    }
}