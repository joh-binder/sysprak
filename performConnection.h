#ifndef PERFORM_CONNECTION
#define PERFORM_CONNECTION

#include "shmfunctions.h"

struct tempPlayerInfo {
    int playerNumber;
    char playerName[1024];
    bool readyOrNot;
};

void receive_msg(int sock, char *recmsg);

void send_msg(int sock, char *sendmsg);

void performConnection(int sock, char *gameID, int playerN, char *gamekindclient, struct gameInfo *pGame, struct playerInfo *pPlayer);

/* Verhalten nach "+ MOVE <<Zugzeit>>" nur in der ersten Runde, d.h. wenn noch kein Shared-Memory-Bereich für die
 * Spielsteine existiert: Liest alle Spielsteine ein und speichert sie zwischen (mit malloc --> Leakgefahr, nochmal checken!).
 * Legt am Ende, wenn die Anzahl der Spielsteine feststeht, Shmemory in der passenden Größe an und überträgt alle
 * Informationen dorthin. Schreibt dann THINKING und empfängt "+ OKTHINK". */
struct line *moveBehaviorFirstRound(int sock, struct gameInfo *pGame);

/* Verhalten nach "+ MOVE <<Zugzeit>>". Liest Spielsteine ein und speichert sie im Shared-Memory-Bereich.
 * Schreibt "THINKING" und wartet auf "+ OKTHINK". */
void moveBehavior(int sock, struct line *pLine, struct gameInfo *pGame);

/* Für das Verhalten nach der Zeile "+ GAMEOVER". Liest ein letztes Mal die Spielsteine ein und speichert sie im Shared
 * Memory; druckt dann eine Botschaft darüber aus, wer gewonnen/verloren hat. */
void gameoverBehavior(int sock, struct line *pLine, struct gameInfo *pGame, struct playerInfo *pPlayer);

/* Liest "+ PIECESLIST", alle Spielsteine und "+ ENDPIECESLIST". Schreibt die Spielsteine in Form von struct line-s
 * in den dafür vorgesehenen Shmemory-Bereich (beginnt bei pLine). Schreibt außerdem in pGame->sizeMoveShmem
 * die Anzahl der gelesenen Spielsteine und setzt die Flag für Thinker, dass neue Info verfügbar ist. */
void processMoves(int sock, struct line *pLine, struct gameInfo *pGame);

#endif