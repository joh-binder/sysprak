#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>

#include "performConnection.h"

#define MAX_LEN 1024
#define CLIENTVERSION "2.0"

char recstring[MAX_LEN];
char bufferstring[MAX_LEN];
static bool moveShmExists = false;

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

void performConnection(int sock, char *gameID, int playerN, char* gamekindclient, struct gameInfo *pGame) {

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
    pGame->ownPlayerNumber = ownPlayerNumber;

    // eigene Spielerinfos abspeichern
    struct playerInfo *pFirstPlayer = playerShmalloc();
    if (pFirstPlayer == (struct playerInfo *)NULL) {
        fprintf(stderr, "Fehler bei der Zuteilung von Shared Memory für die eigenen Spielerinfos.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    *pFirstPlayer = createPlayerInfoStruct(ownPlayerNumber, playerName, true);

    // andere Spielerinfos abspeichern -> als Schleife, damit es auch für Spiele mit mehr als zwei Spielern funktioniert
    for (int i = 0; i < totalplayer-1; i++) {
        struct playerInfo *pNextPlayer = playerShmalloc();
        if (pNextPlayer == (struct playerInfo *)NULL) {
            fprintf(stderr, "Fehler bei der Zuteilung von Shared Memory für Infos eines anderen Spielers.\n");
            close(sock);
            exit(EXIT_FAILURE);
        } else {
            *pNextPlayer = createPlayerInfoStruct(oppInfo[i].playerNumber, oppInfo[i].playerName, oppInfo[i].readyOrNot);
        }
    }

    // ENDE DES PROLOGS
    // AB JETZT NORMALER SPIELVERLAUF

    int timeForMove;
    struct line *pMoveInfo;

    /* Schleife, liest eine Zeile vom Server und handelt entsprechend: "+ WAIT" wird sofort beantwortet,
     * "+ MOVE <<Zugzeit>>" und "+ GAMEOVER" werden mit entsprechenden Funktionsaufrüfen behandelt. Sonst sollten keine
     * Nachrichten kommen (außer Fehler). */
    while (pGame->isActive) {
        printf("Neue Iteration\n"); // nur für mich, kann später weg
        receive_msg(sock, recstring);
        printf("In ingameBehavior: %s\n", recstring); // nur für mich, kann später weg

        if (strcmp(recstring+2, "WAIT") == 0) {
            printf("Ich muss jetzt das WAIT beantworten!\n"); // nur für mich aus Paranoia, kann später weg
            send_msg(sock, "OKWAIT\n");
        } else if (strcmp(recstring+2, "GAMEOVER") == 0) {
            pGame->isActive = false;
            gameoverBehavior(sock, pMoveInfo, pGame);
        } else {
            /* ansonsten sollte recstring+2 die Form "WORT ZAHL" haben. Versuche, WORT nach recstring und Zahl nach
             * timeForMove zu schreiben. Wenn das nicht geht (falsches Format?) -> Fehler. */
            if (sscanf(recstring+2, "%s %d", recstring, &timeForMove) != 2) {
                fprintf(stderr, "Fehler beim Auslesen einer Spielverlaufsnachricht.\n");
                close(sock);
                exit(EXIT_FAILURE);
            }
            // Überprüfe, ob das WORT auch wirklich MOVE ist -> nur dann in moveBehavior gehen
            if (strcmp(recstring, "MOVE") == 0) {
                if (!moveShmExists) {
                    pMoveInfo = moveBehaviorFirstRound(sock, pGame);
                    moveShmExists = true;
                } else {
                    moveBehavior(sock, pMoveInfo, pGame);
                }
            } else {
                fprintf(stderr, "Fehler! Unbekannte Nachricht erhalten: %s.\n", recstring);
                close(sock);
                exit(EXIT_FAILURE);
            }

        }

    }

}

/* Verhalten nach "+ MOVE <<Zugzeit>>" nur in der ersten Runde, d.h. wenn noch kein Shared-Memory-Bereich für die
 * Spielsteine existiert: Liest alle Spielsteine ein und speichert sie zwischen (mit malloc --> Leakgefahr, nochmal checken!).
 * Legt am Ende, wenn die Anzahl der Spielsteine feststeht, Shmemory in der passenden Größe an und überträgt alle
 * Informationen dorthin. Schreibt dann THINKING und empfängt "+ OKTHINK". */
struct line *moveBehaviorFirstRound(int sock, struct gameInfo *pGame) {

    // empfängt "+ PIECESLIST"
    receive_msg(sock, recstring);

    int counter = 0; // zählt die empfangenen Zeilen = Spielsteine
    bool isReadingMoves = true;
    struct line *pTempMemoryForMoves = malloc(sizeof(struct line)); // Speicher für EIN struct line

    while (isReadingMoves) {
        receive_msg(sock, recstring);
        if (strcmp(recstring+2, "ENDPIECESLIST") == 0) { // Abbruch der Schleife
            isReadingMoves = false;
        } else {
            pTempMemoryForMoves = (struct line *)realloc(pTempMemoryForMoves, (counter+1) * sizeof(struct line)); // Platz für ein struct line mehr von realloc holen
            pTempMemoryForMoves[counter] = createLineStruct(recstring+2); // den empfangenen String in den gereallocten Speicher schreiben
            counter++;
        }
    }

    pGame->sizeMoveShmem = counter; // Anzahl der Spielsteininfos im Shmemory aktualisieren

    // erzeugt einen Shared-Memory-Bereich in passender Größe für alle Spielsteine (als struct line)
    int shmidMoveInfo = createShmemoryForMoves(counter);
    struct line *ret = shmAttach(shmidMoveInfo);

    // überträgt alle Spielsteininfos vom gemallocten Zwischenspeicher in das neue Shmemory
    for (int i = 0; i < counter; i++) {
        ret[i] = createLineStruct(pTempMemoryForMoves[i].line);
    }

    pGame->newMoveInfoAvailable = true;

    printf("Ich muss jetzt THINKING schreiben!\n"); // nur für mich, kann später weg
    send_msg(sock, "THINKING\n");

    // Empfängt "+ OKTHINK"
    receive_msg(sock, recstring);

    // gemallocter Zwischenspeicher kann wieder freigegeben werden
    free(pTempMemoryForMoves);
    pTempMemoryForMoves = NULL;

    return ret;
}

/* Verhalten nach "+ MOVE <<Zugzeit>>". Liest Spielsteine ein und speichert sie im Shared-Memory-Bereich.
 * Schreibt "THINKING" und wartet auf "+ OKTHINK". */
void moveBehavior(int sock, struct line *pLine, struct gameInfo *pGame) {

    // liest alle Spielsteine ein und schreibt sie ins Shmemory
    processMoves(sock, pLine, pGame);

    printf("Ich muss jetzt THINKING schreiben!\n"); // nur für mich, kann später weg
    send_msg(sock, "THINKING\n");

    // Empfängt "+ OKTHINK"
    receive_msg(sock, recstring);


}

/* Für das Verhalten nach der Zeile "+ GAMEOVER". Liest ein letztes Mal die Spielsteine ein und speichert sie im Shared
 * Memory; druckt dann eine Botschaft darüber aus, wer gewonnen/verloren hat. */
void gameoverBehavior(int sock, struct line *pLine, struct gameInfo *pGame) {

    // liest alle Spielsteine ein und schreibt sie ins Shmemory
    processMoves(sock, pLine, pGame);

    // jetzt kommt "+ PLAYER0WON Yes/No" für jeden Spieler
    for (unsigned int i = 0; i < pGame->numberOfPlayers; i++) {
        receive_msg(sock, recstring);
        // wir wollen eigentlich nur Yes/No in recstring haben, Fehler wenn das nicht geht
        if (sscanf(recstring, "*[^ ] %s", recstring) != 1) {
            fprintf(stderr, "Fehler beim Verarbeiten der Informationen, welcher Spieler gewonnen hat.\n");
            close(sock);
            exit(EXIT_FAILURE);
        } else {
            if (getPlayerFromNumber(i) == NULL) {
                fprintf(stderr, "Fehler! Habe ein Ergebnis für Spieler %d erhalten, aber zu dieser Nummer gibt es keine Spielerdaten.\n", i);
                close(sock);
                exit(EXIT_FAILURE);
            } else {
                if (strcmp(recstring, "Yes") == 0) {
                    printf("%s (Spieler %d) hat gewonnen.\n", getPlayerFromNumber(i)->playerName, i);
                } else if (strcmp(recstring, "No") == 0) {
                    printf("%s (Spieler %d) hat verloren.\n", getPlayerFromNumber(i)->playerName, i);
                } else {
                    fprintf(stderr, "Fehler! Spieler %d hat folgendes Spielergebnis, welches nicht interpretiert werden konnte: %s\n", i, recstring);
                    close(sock);
                    exit(EXIT_FAILURE);
                }
            }
        }

        printf("In gameoverBehavior: %s\n", recstring);
    }

    // empfängt "+ QUIT"
    receive_msg(sock, recstring);

}

/* Liest "+ PIECESLIST", alle Spielsteine und "+ ENDPIECESLIST". Schreibt die Spielsteine in Form von struct line-s
 * in den dafür vorgesehenen Shmemory-Bereich (beginnt bei pLine). Schreibt außerdem in pGame->sizeMoveShmem
 * die Anzahl der gelesenen Spielsteine und setzt die Flag für Thinker, dass neue Info verfügbar ist. */
void processMoves(int sock, struct line *pLine, struct gameInfo *pGame) {

    int counter = 0;
    bool isReadingMoves = true;

    // empfängt "+ PIECESLIST"
    receive_msg(sock, recstring);

    while (isReadingMoves) {
        receive_msg(sock, recstring);
        if (strcmp(recstring+2, "ENDPIECESLIST") == 0) {
            isReadingMoves = false;
        } else {
            printf("Gelesen: %s\n", recstring);
            pLine[counter] = createLineStruct(recstring+2);
            counter++;
        }
    }

    pGame->sizeMoveShmem = counter;
    pGame->newMoveInfoAvailable = true;





}