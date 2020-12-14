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

void performConnection(int sock, char *gameID, int playerN, char *gamekindclient, struct gameInfo *pGame, struct playerInfo *pPlayer, int maxNumPlayersInShmem);

#endif