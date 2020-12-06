#ifndef SYSPRAK_SHMFUNCTIONS_H
#define SYSPRAK_SHMFUNCTIONS_H

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
    int readyOrNot; // 0 or 1
    struct playerInfo *nextPlayerPointer;
    int ownShmid;
};

// entfernt alle für einzelnen Spieler angelegten Shared-Memory-Bereiche; gibt 0 bei Erfolg und -1 im Fehlerfall zurück
int deletePlayerShm(struct playerInfo *startPlayer);

// erzeugt ein neues struct gameInfo und initialisiert es mit Standardwerten
struct gameInfo createGameInfoStruct(void);

// erzeugt ein neues struct gameInfo und initialisiert es mit übergebenen Werten für playerNumber, playerName und readyOrNot
struct playerInfo createPlayerInfoStruct(int pN, char *name, int ready, int shmid);

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
