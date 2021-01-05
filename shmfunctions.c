#include <sys/shm.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "shmfunctions.h"

static int playerShmallocCounter = 0;
static unsigned int sizeOfPlayerShmallocBlock = MAX_NUMBER_OF_PLAYERS_IN_SHMEM * sizeof(struct playerInfo);
static struct playerInfo *pStartPlayer;

static unsigned int sizeOfMoveShmallocBlock;


// erzeugt ein neues struct gameInfo und initialisiert es mit Standardwerten
struct gameInfo createGameInfoStruct(void) {
    struct gameInfo ret;
    memset(ret.gameKindName, 0, MAX_LENGTH_NAMES);
    memset(ret.gameName, 0, MAX_LENGTH_NAMES);
    ret.numberOfPlayers = 0;
    ret.ownPlayerNumber = -1;
    ret.pidConnector = 0;
    ret.pidThinker = 0;
    ret.sizeMoveShmem = 0;
    ret.newMoveInfoAvailable = false;
    ret.isActive = true;
    return ret;
}

// erzeugt ein neues struct gameInfo und initialisiert es mit übergebenen Werten für playerNumber, playerName und readyOrNot
struct playerInfo createPlayerInfoStruct(int pN, char *name, bool ready) {
    struct playerInfo ret;
    ret.playerNumber = pN;
    strncpy(ret.playerName, name, MAX_LENGTH_NAMES);
    ret.readyOrNot = ready;
    return ret;
}

/* Nimmt einen String als Parameter und kapselt ihn (bzw. max. MOVE_LINE_BUFFER-1 Zeichen davon) in einem struct line. */
struct line createLineStruct(char *content) {
    struct line ret;
    memset(ret.line, 0, MOVE_LINE_BUFFER);
    strncpy(ret.line, content, MOVE_LINE_BUFFER-1);
    return ret;
}

/* In der Main-Methode sollte ein Shared-Memory-Bereich für alle struct playerInfos angelegt worden sein. Danach muss
 * der Pointer darauf einmal mit dieser Funktion an diese Methode übergeben werden, damit hier die statische Variable
 * entsprechend gesetzt werden kann, welche dann andere Funktionen benutzen. */
void setUpPlayerAlloc(struct playerInfo *pStart) {
    pStartPlayer = pStart;
}

/* Überprüft, ob der Speicherplatz, der im Voraus für struct PlayerInfos reserviert wurde, überhaupt groß genug ist
 * für die Spieleranzahl, die der Server mitteilt. Falls ja, 0, falls nein, -1. */
int checkPlayerShmallocSize(int size) {
    if (size > MAX_NUMBER_OF_PLAYERS_IN_SHMEM) return -1;
    else return 0;
}

/* Setzt voraus, dass ein Speicherblock für struct playerInfos reserviert ist. Gibt einen Pointer auf einen Abschnitt
 * des Speicherblocks zurück, der genau groß genug für eine struct playerInfo ist. Intern wird ein Zähler
 * versetzt, sodass der nächste Funktionsaufruf einen anderen Abschnitt liefert. */
struct playerInfo *playerShmalloc() {
    if (sizeOfPlayerShmallocBlock < (playerShmallocCounter + 1) * sizeof(struct playerInfo)) {
        fprintf(stderr, "Fehler! Speicherblock ist bereits voll. Kann keinen Speicher mehr zuteilen.\n");
        return (struct playerInfo *)NULL;
    } else {
        struct playerInfo *pTemp = pStartPlayer + playerShmallocCounter;
        playerShmallocCounter += 1;
        return pTemp;
    }
}

/* Durchsucht eine Liste von struct playerInfos nach einer gegebenen Spielernummer, gibt einen
 * Pointer auf das struct des entsprechenden Spielers zurück, falls vorhanden; ansonsten Nullpointer */
struct playerInfo *getPlayerFromNumber(int targetNumber) {
    struct playerInfo *pCurrentPlayer = pStartPlayer;
    while (pCurrentPlayer != NULL) {
        if (pCurrentPlayer->playerNumber == targetNumber) {
            return pCurrentPlayer;
        } else {
            pCurrentPlayer += 1;
        }
    }
    return NULL;
}

/* Erzeugt einen Shmemory-Bereich, der groß genug ist, um MAX_NUMBER_OF_PLAYERS_IN_SHMEM Stück struct playerInfo
 * aufnehmen zu können. Gibt die Shm-ID zurück, oder -1 im Fehlerfall. */
int createShmemoryForPlayers(void) {
    int shmid = shmCreate(sizeOfPlayerShmallocBlock * sizeof(struct playerInfo));
    return shmid;
}

/* Erzeugt einen Shared-Memory-Bereich, der groß genug ist, um numOfLines Stück struct line aufnehmen zu können.
110  * Gibt die Shm-ID zurück, oder -1 im Fehlerfall. */
int createShmemoryForMoves(int numOfLines) {
	sizeOfMoveShmallocBlock = numOfLines * sizeof(struct line);

	int shmid = accessExistingMoveShmem();

	if (shmid < 0) {
		shmid = shmget(ftok("main.c", KEY_FOR_MOVE_SHMEM), sizeOfMoveShmallocBlock, IPC_CREAT | IPC_EXCL | SHM_R | SHM_W);
		if (shmid < 0) {
			perror("Fehler bei Shared-Memory-Erstellung");
		}
		printf("(angelegt) Shared-Memory-ID: %d\n", shmid); // nur zur Kontrolle, kann weg
	}
	return shmid;
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


/* Für den Thinker: Greift auf den Shared-Memory-Bereich zu, der im Connector erstellt wurde.
 * Gibt die Shm-ID zurück, oder -1 im Fehlerfall. */
int accessExistingMoveShmem(void) {
    int shmid = shmget(ftok("main.c", KEY_FOR_MOVE_SHMEM), sizeOfMoveShmallocBlock, IPC_EXCL | SHM_R | SHM_W);
    if (shmid < 0) {
        printf("Shared-Memory für Spielzüge existiert noch nicht.\n");
    }
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
