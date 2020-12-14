#ifndef SYSPRAK_SHMFUNCTIONS_H
#define SYSPRAK_SHMFUNCTIONS_H

#include <stdbool.h>
#define MAX_LENGTH_NAMES 256

struct gameInfo {
    char gameName[MAX_LENGTH_NAMES];
    int playerNumber;
    unsigned int numberOfPlayers;
    pid_t pidConnector;
    pid_t pidThinker;
};

struct playerInfo {
    int playerNumber;
    char playerName[MAX_LENGTH_NAMES];
    bool readyOrNot;
    struct playerInfo *nextPlayerPointer;
};


// erzeugt ein neues struct gameInfo und initialisiert es mit Standardwerten
struct gameInfo createGameInfoStruct(void);

// erzeugt ein neues struct gameInfo und initialisiert es mit übergebenen Werten für playerNumber, playerName und readyOrNot
struct playerInfo createPlayerInfoStruct(int pN, char *name, bool ready);

/* Nimmt als Parameter einen Pointer auf den Anfang eines (Shared-Memory-)Speicherblocks, die Blockgröße
 * und eine gewünschte Größe. Reserviert dann in diesem Speicherblock einen Abschnitt in der gewünschten
 * Größe und gibt einen Pointer darauf zurück. Gibt NULL zurück, falls nicht genügend Platz vorhanden.*/
void *playerShmalloc(void *pointerToStart, int desiredSize, int maxBlockSize);

/* Durchsucht eine Liste von struct playerInfos nach einer gegebenen Spielernummer, gibt einen
 * Pointer auf das struct des entsprechenden Spielers zurück, falls vorhanden; ansonsten Nullpointer;
 * Eingabeparameter sind ein Pointer auf ein struct playerInfo, ab dem gesucht werden soll (i.d.R. das erste Struct der
 * Liste) sowie die gewünschte Spielernummer
 */
struct playerInfo *getPlayerFromNumber(struct playerInfo *pCurrentPlayer, int targetNumber);

// Erzeugt ein Shared-Memory-Segment einer gegebenen Größe und gibt dessen ID zurück, oder -1 im Fehlerfall.
int shmCreate(int shmdatasize);

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
