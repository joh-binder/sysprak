#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#include "performConnection.h"

#define MAX_LEN 1024
#define CLIENTVERSION "2.0"

char recstring[MAX_LEN];
char bufferstring[MAX_LEN];


void receive_msg(int sock, char *recmsg){

    int shiftInRecmsg = 0;
    int testrec;

    memset(bufferstring, 0, strlen(bufferstring));

    do {
      testrec = recv(sock, bufferstring, 1, 0);

      //Testen, ob Bytes empfangen wurden
      if(testrec < 0){
          perror("Fehler beim Nachrichtenempfang vom Server.\n");
          close(sock);
          exit(EXIT_FAILURE);
      } else if(testrec == 0){
          perror("Server hat Verbindung abgebrochen.\n");
          close(sock);
          exit(EXIT_FAILURE);
      } else {
        strcpy(recmsg+shiftInRecmsg, bufferstring);
        shiftInRecmsg += testrec;
      }

    } while (bufferstring[testrec-1] != '\n');

    //Entfernen von \n
    recmsg[shiftInRecmsg-1] = '\0';

    //Nur zum Testen eingebaut
    // printf("recmsg ist: %s\n", recmsg);


    if (strlen(recmsg) > 1 && *recmsg == '-') {

        //Fehlerbehandlung bei falschen Angaben
        if (strcmp(recmsg + 2, "TIMEOUT Be faster next time") == 0) {
            fprintf(stderr, "Fehler! Zu langsam. Versuche beim nächsten Mal schnneller zu sein. (Server: %s)\n", recmsg + 2);
        } else if (strcmp(recmsg + 2, "Not a valid game ID") == 0) {
            fprintf(stderr, "Fehler! Keine zulässige Game ID. (Server: %s)\n", recmsg + 2);
        } else if (strcmp(recmsg + 2, "Game does not exist") == 0) {
            fprintf(stderr, "Fehler! Dieses Spiel existiert nicht. (Server: %s)\n", recmsg + 2);
        } else if (strcmp(recmsg + 2, "No free player") == 0) {
            fprintf(stderr, "Fehler! Spieler ist nicht verfügbar. (Server: %s)\n", recmsg + 2);
        } else if (strcmp(recmsg + 2, "Client Version does not match server Version") == 0) {
            fprintf(stderr, "Fehler! Die Client Version und Server Version stimmen nicht überein. (Server: %s)\n", recmsg + 2);
        } else {
            fprintf(stderr, "Es ist ein unerwarteter Fehler aufgetreten. (Server: %s)\n", recmsg + 2);
        }

        close(sock);
        exit(EXIT_FAILURE);
    }

}

//Client Nachricht senden
void send_msg(int sock, char *sendmsg) {
    int bytesmsg = strlen(sendmsg);
    int bytessend = send(sock, sendmsg, bytesmsg, 0);

    if (bytesmsg != bytessend) {
        fprintf(stderr, "Fehler! Bytes der zu sendenden Nachricht: %d, aber gesendete Bytes: %d", bytesmsg, bytessend);
    }
}

void prettyPrint(char *gameKind, char *gameID, char *playerName, int totalPlayer, struct tempPlayerInfo *oppInfo) {
    printf("=========================================\n");
    printf("Ihr spielt: %s\n", gameKind);
    printf("Die ID eures Spiels lautet: %s\n", gameID);
    printf("Du bist: %s\n", playerName);
    printf("Die Gesamtanzahl an Spielern ist: %d\n", totalPlayer);
    if (totalPlayer > 2) printf("Informationen zu den Gegnern:\n");
    else printf("Informationen zum Gegner:\n");
    for (int i = 0; i < totalPlayer - 1; i++) {
      printf("Gegner %d hat die Nummer: %d\n", i + 1, oppInfo[i].playerNumber);
      printf("Gegner %d heißt: %s\n", i + 1, oppInfo[i].playerName);
      if (oppInfo[i].readyOrNot) printf("Gegner %d ist bereit.\n", i + 1);
      else printf("Gegner %d ist nicht bereit.\n", i + 1);
    }
    printf("=========================================\n");
}

void performConnection(int sock, char *gameID, int playerN, char* gamekindclient, struct gameInfo *pGame, struct playerInfo *pPlayer, int maxNumPlayersInShmem) {

    char gamekindserver[MAX_LEN];
    char gamename[MAX_LEN];
    char playerName[MAX_LEN];
    int totalplayer, ownPlayerNumber;

    if (sock <= 0) {
        perror("Fehler! Socket konnte nicht erstellt werden.\n");
    }

    char buffer[MAX_LEN];

    //Empfängt Nachricht mit der Serverversion und schickt als Antwort die Clientversion
    receive_msg(sock, recstring);
    sprintf(buffer, "VERSION %s\n", CLIENTVERSION);
    send_msg(sock, buffer);

    //Empfängt die Nachricht, ob der Server die Client Version akzeptiert hat
    receive_msg(sock, recstring);


    //Schickt dem Server die Game ID
    sprintf(buffer, "ID %s\n", gameID);

    //strcpy(gamekindserver, buffer + 10);
    send_msg(sock, buffer);

    //Empfängt bei valider Game ID die Art des Spieles vom Server
    receive_msg(sock, recstring);

    //reduziert die Server Nachricht auf gamekind
    strncpy(gamekindserver, recstring + 10, MAX_LEN);

    for (unsigned long i = 0; i <= strlen(gamekindserver); i++) {
        if (i == '\n') {
            gamekindserver[i-4] = '\0';
        }
    }

    if (strcmp(gamekindserver, gamekindclient) == 0) {
        if (playerN == -1) {
            sprintf(buffer, "PLAYER\n");
            send_msg(sock, buffer);

        } else {
            sprintf(buffer, "PLAYER %d\n", playerN);
            send_msg(sock, buffer);
        }
    } else {
        fprintf(stderr, "Fehler! Falsche Spielart. Spiel des Clients: %s. Spiel des Servers: %s. Client wird beendet.\n", gamekindclient, gamekindserver);
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Empfängt den Namen des Spiels
    receive_msg(sock, recstring);
    strncpy(gamename, recstring+2, MAX_LEN-1);

    // Empfängt die Zeile "+ YOU <<Mitspielernummer>> <<Mitspielername>>"
    receive_msg(sock, recstring);
    if (sscanf(recstring+6, "%d %[^\n]", &ownPlayerNumber, playerName) != 2) {
        fprintf(stderr, "Fehler beim Verarbeiten der eigenen Spielerinformationen\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Empfängt die Zeile "+ TOTAL Mitspieleranzahl"
    receive_msg(sock, recstring);
    if (sscanf(recstring+2, "%*[^ ] %d", &totalplayer) != 1) {
      fprintf(stderr, "Fehler beim Verarbeiten der Mitspieleranzahl\n");
      close(sock);
      exit(EXIT_FAILURE);
    }

    struct tempPlayerInfo oppInfo[totalplayer-1];

    for (int i = 0; i < totalplayer-1; i++) {

      receive_msg(sock, recstring); // in recstring steht jetzt "+ <<Mitspielernummer>> <<Mitspielername>> <<Bereit>>"

      // trennt am Leerzeichen nach Mitspielernummer; Mitspielernummer wird in playerNumber geschrieben; alles dahinter zurück in recstring
      if (sscanf(recstring+2, "%d %[^\n]", &oppInfo[i].playerNumber, recstring) != 2) {
        fprintf(stderr, "Fehler beim Verarbeiten von Mitspielerinformationen\n");
        close(sock);
        exit(EXIT_FAILURE);
      }

      // schreibt recstring außer die letzten zwei Zeichen nach playerName
      strncpy(oppInfo[i].playerName, recstring, strlen(recstring)-2);
      oppInfo[i].playerName[strlen(recstring)-2] = '\0';

      // betrachten noch das letzte Zeichen (= Bereit)
      if (recstring[strlen(recstring)-1] == '0') {
        oppInfo[i].readyOrNot = false;
      } else if (recstring[strlen(recstring)-1] == '1') {
        oppInfo[i].readyOrNot = true;
      } else {
        fprintf(stderr, "Fehler beim Verarbeiten von Mitspielerinformationen: Bereit-Wert konnte nicht interpretiert werden\n");
        close(sock);
        exit(EXIT_FAILURE);
      }

    }

    receive_msg(sock, recstring);
    if (strcmp(recstring, "+ ENDPLAYERS") != 0) {
      fprintf(stderr, "Fehler! Mehr Spieler-Infos empfangen als erwartet. Client wird beendet.\n");
      close(sock);
      exit(EXIT_FAILURE);
    }

    prettyPrint(gamekindserver, gameID, playerName, totalplayer, oppInfo);

    // schreibt die empfangenen Daten in den Shared-Memory-Bereich
    // zuerst die allgemeinen Spielinfos
    *pGame = createGameInfoStruct();
    strncpy(pGame->gameKindName, gamekindserver, MAX_LENGTH_NAMES);
    strncpy(pGame->gameName, gamename, MAX_LENGTH_NAMES);
    pGame->numberOfPlayers = totalplayer;

    // testen, ob der Shmemory-Bereich überhaupt groß genug für alle Spieler ist
    if (totalplayer > maxNumPlayersInShmem) {
        fprintf(stderr, "Fehler! Im Shared-Memory-Bereich ist nicht genug Platz, um die Informationen für alle Spieler abzuspeichern.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // eigene Spielerinfos abspeichern
    struct playerInfo *pFirstPlayer = playerShmalloc(pPlayer ,maxNumPlayersInShmem * sizeof(struct gameInfo));
    if (pFirstPlayer == (struct playerInfo *)NULL) {
        fprintf(stderr, "Fehler bei der Zuteilung von Shared Memory für die eigenen Spielerinfos.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    *pFirstPlayer = createPlayerInfoStruct(ownPlayerNumber, playerName, true);

    // andere Spielerinfos abspeichern -> als Schleife, damit es auch für Spiele mit mehr als zwei Spielern funktioniert
    struct playerInfo *pPreviousPlayer = pFirstPlayer;
    struct playerInfo *pCurrentPlayer;

    for (int i = 0; i < totalplayer-1; i++) {
        pCurrentPlayer = playerShmalloc(pPlayer, maxNumPlayersInShmem * sizeof(struct gameInfo));
        if (pCurrentPlayer == (struct playerInfo *)NULL) {
            fprintf(stderr, "Fehler bei der Zuteilung von Shared Memory für Infos eines anderen Spielers.\n");
            close(sock);
            exit(EXIT_FAILURE);
        }
        *pCurrentPlayer = createPlayerInfoStruct(oppInfo[0].playerNumber, oppInfo[0].playerName, oppInfo[0].readyOrNot);
        pPreviousPlayer->nextPlayerPointer = pCurrentPlayer;
        pPreviousPlayer = pCurrentPlayer;
    }


}