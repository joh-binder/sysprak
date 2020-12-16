#ifndef SYSPRAK_SHMFUNCTIONS_H
#define SYSPRAK_SHMFUNCTIONS_H

#include <stdbool.h>
#include "thinkerfunctions.h"

#define MAX_LENGTH_NAMES 256
#define MAX_NUMBER_OF_PLAYERS_IN_SHMEM 10
#define KEY_FOR_MOVE_SHMEM 1234567
#define MOVE_LINE_BUFFER 10

struct gameInfo {
    char gameKindName[MAX_LENGTH_NAMES];
    char gameName[MAX_LENGTH_NAMES];
    unsigned int numberOfPlayers;
    int ownPlayerNumber; // praktisch, wenn die hier nochmal steht
    pid_t pidConnector;
    pid_t pidThinker;
    int sizeMoveShmem; // d.h. Anzahl an struct line, für die das Spielzeug-Shmemory erstellt wurde
    bool newMoveInfoAvailable;
    bool isActive; // = noch nicht Game Over
};

struct playerInfo {
    int playerNumber;
    char playerName[MAX_LENGTH_NAMES];
    bool readyOrNot;
    struct playerInfo *nextPlayerPointer;
};

struct line {
    char line[MOVE_LINE_BUFFER];
};

// erzeugt ein neues struct gameInfo und initialisiert es mit Standardwerten
struct gameInfo createGameInfoStruct(void);

// erzeugt ein neues struct gameInfo und initialisiert es mit übergebenen Werten für playerNumber, playerName und readyOrNot
struct playerInfo createPlayerInfoStruct(int pN, char *name, bool ready);

/* Nimmt einen String als Parameter und kapselt ihn (bzw. max. MOVE_LINE_BUFFER-1 Zeichen davon) in einem struct line. */
struct line createLineStruct(char *content);

/* Nimmt als Parameter einen Pointer auf den Anfang eines (Shared-Memory-)Speicherblocks, die Blockgröße
 * und eine gewünschte Größe. Reserviert dann in diesem Speicherblock einen Abschnitt in der gewünschten
 * Größe und gibt einen Pointer darauf zurück. Gibt NULL zurück, falls nicht genügend Platz vorhanden.*/
struct playerInfo *playerShmalloc(struct playerInfo *pointerToStart);


/* Durchsucht eine Liste von struct playerInfos nach einer gegebenen Spielernummer, gibt einen
 * Pointer auf das struct des entsprechenden Spielers zurück, falls vorhanden; ansonsten Nullpointer;
 * Eingabeparameter sind ein Pointer auf ein struct playerInfo, ab dem gesucht werden soll (i.d.R. das erste Struct der
 * Liste) sowie die gewünschte Spielernummer
 */
struct playerInfo *getPlayerFromNumber(struct playerInfo *pCurrentPlayer, int targetNumber);

/* Erzeugt einen Shmemory-Bereich, der groß genug ist, um MAX_NUMBER_OF_PLAYERS_IN_SHMEM Stück struct playerInfo
 * aufnehmen zu können. Gibt die Shm-ID zurück, oder -1 im Fehlerfall. */
int createShmemoryForPlayers(void);

/* Erzeugt einen Shared-Memory-Bereich, der groß genug ist, um numOfLines Stück struct line aufnehmen zu können.
 * Gibt die Shm-ID zurück, oder -1 im Fehlerfall. */
int createShmemoryForMoves(int numOfLines);

// Erzeugt ein Shared-Memory-Segment einer gegebenen Größe und gibt dessen ID zurück, oder -1 im Fehlerfall.
int shmCreate(int shmdatasize);

/* Für den Thinker: Greift auf den Shared-Memory-Bereich zu, der im Connector erstellt wurde.
 * Gibt die Shm-ID zurück, oder -1 im Fehlerfall. */
int accessExistingMoveShmem(void);

/* Bindet das Shared-Memory-Segment einer gegebenen ID an den Adressraum an und gibt einen Pointer auf die
 * Anfangsadresse zurück, oder (void *) -1 im Fehlerfall. */
void *shmAttach(int shmid);

/* Bindet das Shared-Memory-Segment zu einem gegebenen Pointer wieder vom Adressraum ab.
 * Gibt 0 im Erfolgsfall und -1 im Fehlerfall zurück. */
int shmDetach(void *shmpointer);

/* Löscht das Shared-Memory-Segment zu einer gegebenen ID.
 * Gibt 0 im Erfolgsfall und -1 im Fehlerfall zurück. */
int shmDelete(int shmid);

#endif //SYSPRAK_SHMFUNCTIONS_H
