#ifndef MAINLOOP
#define MAINLOOP

#include "shmfunctions.h"

#define GAME_ID_LENGTH 13

struct tempPlayerInfo {
    int playerNumber;
    char playerName[1024];
    bool readyOrNot;
};

//mainloop_epoll muss für den Aufruf in Main.c übergeben werden.
void mainloop_epoll(int sockfd, int pipefd, char ID[GAME_ID_LENGTH + 1], int playerNum);

/* Die Pointer auf den Shared-Memory-Bereich für die allgemeinen Spielinfos und die Infos zu den einzelnen Spielern
 * müssen einmalig von main an performConnection übergeben werden, damit er dort verfügbar ist. */
void setUpShmemPointers(struct gameInfo *pGeneral, struct playerInfo *pPlayers);

#endif
