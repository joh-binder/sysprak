#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "mainloop.h"
#include "util.h"

#define MAX_LEN 1024
#define MAX_EVENTS 2
#define MOVE 5
#define CLIENTVERSION "2.0"
#define GAMEKINDNAME "Bashni"

typedef struct
{
    char buffer[MAX_LEN];
    int filled;
    void (*line)(char*);
} LineBuffer;

int sockfiled;
int pipefiled;
char gameID[14];
int wantedPlayerNumber;

//Epoll struct um Server und Pipe gleichzeitig auf eingehende Nachrichten zu überprüfen
struct epoll_event eventpipe, eventsock, events[MAX_EVENTS];
int epollfd, nfds;
int counter = 0;

// Pointer auf die Shared-Memory-Bereiche
static struct gameInfo *pGeneralInfo;
static struct playerInfo *pPlayerInfo;

// hier kommen die Infos zu den Gegnern rein
static struct tempPlayerInfo oppInfo[MAX_NUMBER_OF_PLAYERS_IN_SHMEM-1];
int countOpponents = 0;

int countPieceLines = 0;
struct line *pTempMemoryForPieces;

static bool moveShmExists = false;

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




void mainloop_pipeline(char* line){

    char buffer[MAX_LEN];
    int bytessend;

    strcpy(buffer,"");

    sprintf(buffer, "PLAY %s\n", line);
    bytessend = write(sockfiled, buffer, strlen(buffer));

}




void mainloop_sockline(char* line){

    char buffer[MAX_LEN];
    int bytessend;

    char gamekindserver[MAX_LEN];
    char gamename[MAX_LEN];
    char playerName[MAX_LEN];
    int totalplayer, ownPlayerNumber;


    strcpy(buffer,"");


    if(strlen(line) > 1 && *line == '-'){

        //Fehlerbehandlung bei falschen Angaben
        if(strcmp(line + 2, "TIMEOUT Be faster next time") == 0){
            fprintf(stderr, "Fehler! Zu langsam. Versuche beim nächsten Mal schneller zu sein. (Server: %s)\n", line + 2);
            pGeneralInfo->isActive = 0;
            close(sockfiled);
            close(pipefiled);
            exit(EXIT_FAILURE);
        }else if(strcmp(line + 2, "Not a valid game ID") == 0){
            fprintf(stderr, "Fehler! Keine zulässige Game ID. (Server: %s)\n", line + 2);
            pGeneralInfo->isActive = 0;
            close(sockfiled);
            close(pipefiled);
            exit(EXIT_FAILURE);
        }else if(strcmp(line + 2, "Game does not exist") == 0){
            fprintf(stderr, "Fehler! Dieses Spiel existiert nicht. (Server: %s)\n", line + 2);
            pGeneralInfo->isActive = 0;
            close(sockfiled);
            close(pipefiled);
            exit(EXIT_FAILURE);
        }else if(strcmp(line + 2, "No free player") == 0){
            fprintf(stderr, "Fehler! Spieler ist nicht verfügbar. (Server: %s)\n", line + 2);
            pGeneralInfo->isActive = 0;
            close(sockfiled);
            close(pipefiled);
            exit(EXIT_FAILURE);
        }else if(strcmp(line + 2, "Client Version does not match server Version") == 0){
            fprintf(stderr, "Fehler! Die Client Version und Server Version stimmen nicht überein. (Server: %s)\n", line + 2);
            pGeneralInfo->isActive = 0;
            close(sockfiled);
            close(pipefiled);
            exit(EXIT_FAILURE);
        }else{
            fprintf(stderr, "Es ist ein unerwarteter Fehler aufgetreten. (Server: %s)\n", line + 2);
            pGeneralInfo->isActive = 0;
            close(sockfiled);
            close(pipefiled);
            exit(EXIT_FAILURE);
        }

        exit(EXIT_FAILURE);
    }

    //Erwartet: + MNM Gameserver v2.3 accepting connections
    if(counter == 0){
        counter++;
        sprintf(buffer, "VERSION %s\n", CLIENTVERSION);
        bytessend = write(sockfiled, buffer, strlen(buffer));


    //Erwartet: + Client version accepted - please send Game-ID to join
    }else if(counter == 1){
        counter++;
        sprintf(buffer, "ID %s\n", gameID);
        bytessend = write(sockfiled, buffer, strlen(buffer));


    //Erwartet: + PLAYING Bashni
    }else if(counter == 2){
        counter++;
        strncpy(gamekindserver, line + 10, MAX_LEN);
        if (strcmp(gamekindserver, "Bashni") != 0) {
            fprintf(stderr, "Fehler! Falsche Spielart. Spiel des Clients: %s. Spiel des Servers: %s. Client wird beendet.\n", GAMEKINDNAME, gamekindserver);
            pGeneralInfo->isActive = 0;
            close(sockfiled);
            close(pipefiled);
            exit(EXIT_FAILURE);
        }


    //Erwartet: + <<Game-Name>>
    }else if(counter == 3){
        counter++;
        strncpy(gamename, line+2, MAX_LEN-2);
        gamename[MAX_LEN-1] = '\0';

        if (wantedPlayerNumber == -1) {
            sprintf(buffer, "PLAYER\n");
            bytessend = write(sockfiled, buffer, strlen(buffer));

        } else {
            sprintf(buffer, "PLAYER %d\n", wantedPlayerNumber);
            bytessend = write(sockfiled, buffer, strlen(buffer));
        }

        // überträgt die allgemeinen Spielinfos in den Shmemory-Bereich
        strncpy(pGeneralInfo->gameKindName, gamekindserver, MAX_LENGTH_NAMES);
        strncpy(pGeneralInfo->gameName, gamename, MAX_LENGTH_NAMES);

    //Erwartet: + YOU <<Mitspielernummer>> <<Mitspielername>>
    } else if(counter == 4) {
        counter++;
        if (sscanf(line+6, "%d %[^\n]", &ownPlayerNumber, playerName) != 2) {
            fprintf(stderr, "Fehler beim Verarbeiten der eigenen Spielerinformationen\n");
            pGeneralInfo->isActive = 0;
            close(sockfiled);
            close(pipefiled);
            exit(EXIT_FAILURE);
        }
        //printf("Eigene Spielernummer: %d, Name: %s\n", ownPlayerNumber, playerName);
        pGeneralInfo->ownPlayerNumber = ownPlayerNumber;

        // überträgt diese Informationen in den Shared-Memory-Bereich
        struct playerInfo *pFirstPlayer = playerShmalloc();
        if (pFirstPlayer == (struct playerInfo *)NULL) {
            fprintf(stderr, "Fehler bei der Zuteilung von Shared Memory für die eigenen Spielerinfos.\n");
            pGeneralInfo->isActive = 0;
            close(sockfiled);
            close(pipefiled);
            exit(EXIT_FAILURE);
        }
        *pFirstPlayer = createPlayerInfoStruct(ownPlayerNumber, playerName, true);

    //Erwartet: + TOTAL <<Mitspieleranzahl>>
    } else if(counter == 5) {
        counter++;
        printf("Total: %s\n", line + 2);
        if (sscanf(line+2, "%*[^ ] %d", &totalplayer) != 1) {
            fprintf(stderr, "Fehler beim Verarbeiten der Mitspieleranzahl\n");
            pGeneralInfo->isActive = 0;
            close(sockfiled);
            close(pipefiled);
            exit(EXIT_FAILURE);
        }

        // überprüft, ob für totalplayer Stück Spieler überhaupt genug Shared Memory reserviert wurde
        if (checkPlayerShmallocSize(totalplayer) != 0) {
            fprintf(stderr, "Fehler! Es wird zu wenig Shared-Memory-Platz für %d Spieler bereitgestellt.\n", totalplayer);
            pGeneralInfo->isActive = 0;
            close(sockfiled);
            close(pipefiled);
            exit(EXIT_FAILURE);
        }

        pGeneralInfo->numberOfPlayers = totalplayer; // diese Information muss im Shmemory noch ergänzt werden

    //Erwartet: + <<Mitspielernummer>> <<Mitspielername>> <<Bereit>>
    } else if(counter == 6){
        printf("Zustand 6: %s\n", line + 2); // nur zur Kontrolle

        if (strcmp(line, "+ ENDPLAYERS") != 0) {

            countOpponents++;
            if (countOpponents >= totalplayer) {
                fprintf(stderr, "Fehler! Mehr Spieler-Infos empfangen als erwartet. Client wird beendet.\n");
                pGeneralInfo->isActive = 0;
                close(sockfiled);
                close(pipefiled);
                exit(EXIT_FAILURE);
            } else {
                // trennt am Leerzeichen nach Mitspielernummer; Mitspielernummer wird in playerNumber geschrieben; alles dahinter zurück in line
                if (sscanf(line+2, "%d %[^\n]", &oppInfo[countOpponents-1].playerNumber, line) != 2) {
                    fprintf(stderr, "Fehler beim Verarbeiten von Mitspielerinformationen\n");
                    pGeneralInfo->isActive = 0;
                    close(sockfiled);
                    close(pipefiled);
                    exit(EXIT_FAILURE);
                }
                // schreibt line außer die letzten zwei Zeichen nach playerName
                strncpy(oppInfo[countOpponents-1].playerName, line, strlen(line)-2);
                oppInfo[countOpponents-1].playerName[strlen(line)-2] = '\0';

                // betrachten noch das letzte Zeichen (= Bereit)
                if (line[strlen(line)-1] == '0') {
                    oppInfo[countOpponents-1].readyOrNot = false;
                } else if (line[strlen(line)-1] == '1') {
                    oppInfo[countOpponents-1].readyOrNot = true;
                } else {
                    fprintf(stderr, "Fehler beim Verarbeiten von Mitspielerinformationen: Bereit-Wert konnte nicht interpretiert werden\n");
                    pGeneralInfo->isActive = 0;
                    close(sockfiled);
                    close(pipefiled);
                    exit(EXIT_FAILURE);
                }
            }

        } else {
            // es kommen keine oppInfos mehr -> beim nächsten sockline-Aufrufe in den nächsten Zustand gehen
            counter++;

            // jetzt können wir alle Infos ausdrucken
            prettyPrint(gamekindserver, gameID, playerName, totalplayer, oppInfo);

            // außerdem können wir die oppInfos in Shared-Memory übertragen
            for (int i = 0; i < totalplayer-1; i++) {
                struct playerInfo *pNextPlayer = playerShmalloc();
                if (pNextPlayer == (struct playerInfo *)NULL) {
                    fprintf(stderr, "Fehler bei der Zuteilung von Shared Memory für Infos eines anderen Spielers.\n");
                    pGeneralInfo->isActive = 0;
                    close(sockfiled);
                    close(pipefiled);
                    exit(EXIT_FAILURE);
                } else {
                    *pNextPlayer = createPlayerInfoStruct(oppInfo[i].playerNumber, oppInfo[i].playerName, oppInfo[i].readyOrNot);
                }
            }

        }

    } else if (counter == 7) { // entspricht "normalem" Zustand nach dem Prolog

        if (startsWith(line, "+ WAIT")) {
            sprintf(buffer, "OKWAIT\n");
            bytessend = write(sockfiled, buffer, strlen(buffer));
            printf("Ich habe das WAIT beantwortet.\n"); // nur für mich zur Kontrolle, kann weg
        } else if (startsWith(line, "+ MOVE"))  {
            if (!moveShmExists) {
                counter = 8;
            } else {
                counter = 9;
            }
        } else if (startsWith(line, "+ GAMEOVER")) {

            counter = 10;
        } else {
            fprintf(stderr, "Fehler! Nicht interpretierbare Zeile erhalten: %s\n", line);
            pGeneralInfo->isActive = 0;
            close(sockfiled);
            close(pipefiled);
            exit(EXIT_FAILURE);
        }

    } else if (counter == 8) { // d.h. Zustand beim ersten Mal Move
        if (startsWith(line, "+ PIECESLIST")) {
            pTempMemoryForPieces = malloc(sizeof(struct line));
        } else if (strcmp(line, "+ ENDPIECESLIST") == 0) {
            pGeneralInfo->sizeMoveShmem = countPieceLines; // Anzahl der Spielsteininfos im Shmemory aktualisieren

            // erzeugt einen Shared-Memory-Bereich in passender Größe für alle Spielsteine (als struct line)
            int shmidMoveInfo = createShmemoryForMoves(countPieceLines);
            struct line *ret = shmAttach(shmidMoveInfo);
            moveShmExists = true;

            // überträgt alle Spielsteininfos vom gemallocten Zwischenspeicher in das neue Shmemory
            for (int i = 0; i < countPieceLines; i++) {
                ret[i] = createLineStruct(pTempMemoryForPieces[i].line);
            }

            pGeneralInfo->newMoveInfoAvailable = true;
            free(pTempMemoryForPieces);

            sprintf(buffer, "THINKING\n");
            bytessend = write(sockfiled, buffer, strlen(buffer));
        } else if (strcmp(line, "+ OKTHINK") == 0) {
            counter = 7; // d.h. zurück in den Normalzutand
        } else { // d.h. diese Zeile ist ein Spielstein
            printf("Spielstein (in Move, erste Runde): %s\n", line+2);
            pTempMemoryForPieces = (struct line *)realloc(pTempMemoryForPieces, (countPieceLines+1) * sizeof(struct line)); // Platz für ein struct line mehr von realloc holen
            pTempMemoryForPieces[countPieceLines] = createLineStruct(line+2); // den empfangenen String in den gereallocten Speicher schreiben
            countPieceLines++;
        }

    } else if (counter == 9) { // d.h. Zustand bei Move, nachdem Move-Shmemory schon existiert
        if (startsWith(line, "+ PIECESLIST")) {
            countPieceLines = 0;
        } else if (strcmp(line, "+ ENDPIECESLIST") == 0) {
            pGeneralInfo->newMoveInfoAvailable = true;
            sprintf(buffer, "THINKING\n");
            bytessend = write(sockfiled, buffer, strlen(buffer));
        } else if (strcmp(line, "+ OKTHINK") == 0) {
            counter = 7; // d.h. zurück in den Normalzutand
        } else { // d.h. diese Zeile ist ein Spielstein
            printf("Spielstein (in Move normal): %s\n", line+2);
            pTempMemoryForPieces[countPieceLines] = createLineStruct(line+2);
            countPieceLines++;
        }
    } else if (counter == 10) { // Zustand nach Gameover
        if (startsWith(line, "+ PIECESLIST")) {
            countPieceLines = 0;
        } else if (strcmp(line, "+ ENDPIECESLIST") == 0) {
            pGeneralInfo->newMoveInfoAvailable = true;
        } else if (strcmp(line, "+ QUIT") == 0) {
            pGeneralInfo->isActive = false;
            printf("Das Spiel ist vorbei. Schleife sollte sich jetzt beenden.\n");
        } else if (startsWith(line, "+ PLAYER")) { // erwartet '+ PLAYERXWON Yes/No'
            printf("%s\n", line); // das kann mach noch sauber machen
        } else { // d.h. diese Zeile ist ein Spielstein
            printf("Spielstein (in Move normal): %s\n", line+2);
            pTempMemoryForPieces[countPieceLines] = createLineStruct(line+2);
            countPieceLines++;
        }
    } else {
        fprintf(stderr, "Fehler! Unvorhergesehener Zustand Nr. %d.\n", counter);
        pGeneralInfo->isActive = 0;
        close(sockfiled);
        close(pipefiled);
        exit(EXIT_FAILURE);
    }

}





//Bekommt einen Buffer und macht Zeilen daraus, welche an den LineHandler weitergegeben werden
void mainloop_filehandler(char* buffer, int len, LineBuffer* linebuffer){

    memcpy(linebuffer->buffer+linebuffer->filled, buffer, len);
    linebuffer->filled += len;

    int oldi = 0;
    for(int i = 0; i < linebuffer->filled; i++){
        if(linebuffer->buffer[i] == '\n'){
            linebuffer->buffer[i] = '\0';
            (*(linebuffer->line))(&(linebuffer->buffer[oldi]));
            oldi = i + 1;
        }
    }
    //Kopiert Reststring an den Anfang
    memmove(linebuffer->buffer, &(linebuffer->buffer[oldi]), (linebuffer->filled)-oldi);
    linebuffer->filled = (linebuffer->filled)-oldi;

    //War scheinbar falsch. Steht nur drin bis ich weiß wieso
    //printf("Test_Filehandler: %s\n", linebuffer->buffer);
    //(*(linebuffer->line))(linebuffer->buffer);
}






//Epoll event Loop um Pipe und Socket zu überwachen
void mainloop_epoll(int sockfd, int pipefd[2], char ID[14], int playerNum){

    char buffersock[MAX_LEN];
    char bufferpipe[MAX_LEN];

    strcpy(buffersock,"");
    strcpy(bufferpipe,"");

    sockfiled = sockfd;
    pipefiled = pipefd[2];
    wantedPlayerNumber = playerNum;
    for(long unsigned int i = 0; i < strlen(ID); i++){
        gameID[i] = ID[i];
    }

    //Erstellt zwei Instanzen des LineBuffer Strukts; eins für die Pipe und eins für den Socket
    LineBuffer sockbuffer;
    LineBuffer pipebuffer;
    pipebuffer.filled = 0;
    sockbuffer.filled = 0;
    pipebuffer.line = &mainloop_pipeline;
    sockbuffer.line = &mainloop_sockline;

    //Erstellen der Epoll Instanz
    epollfd = epoll_create1(0);
    if(epollfd == -1){
        perror("Fehler bei epoll_create\n");
    }

    //Daten für die zu überwachende Pipe angeben
    eventpipe.events = EPOLLIN;
    eventpipe.data.fd = pipefd[0];
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, pipefd[0], &eventpipe) == -1){
        perror("Fehler bei epoll_ctl Pipe\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    //Daten für den zu überwachenden Socket angeben
    eventsock.events = EPOLLIN;
    eventsock.data.fd = sockfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &eventsock) == -1){
        perror("Fehler bei epoll_ctl Socket\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    *pGeneralInfo = createGameInfoStruct();

    //Eventloop
    while(pGeneralInfo->isActive){
        //Warten auf Event
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1){
            perror("Fehler bei epoll_wait\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }


        for (int n = 0; n < nfds; ++n){
            //Lesen aus der Pipe
            if (events[n].data.fd == pipefd[0]){

                int bufferlen = read(pipefd[0], bufferpipe, MAX_LEN);
                if(bufferlen != MOVE){
                    perror("Fehler beim Lesen aus der Pipe.\n");
                    pGeneralInfo->isActive = 0;
                    close(sockfiled);
                    close(pipefiled);
                    exit(EXIT_FAILURE);
                }else{
                    mainloop_filehandler(bufferpipe, bufferlen, &pipebuffer);
                }

            //Lesen vom Socket
            }else if (events[n].data.fd == sockfiled){

                int bufferlen = read(sockfd, buffersock, MAX_LEN);
                mainloop_filehandler(buffersock, bufferlen, &sockbuffer);

            }
        }
    }
    //Fehlerbehandlung, wenn closen für den Epollfd fehlschlägt
    if(close(epollfd)){
        perror("Fehler bei close(epollfd)\n");
        pGeneralInfo->isActive = 0;
        close(sockfiled);
        close(pipefiled);
        exit(EXIT_FAILURE);
    }

}

/* Die Pointer auf den Shared-Memory-Bereich für die allgemeinen Spielinfos und die Infos zu den einzelnen Spielern
 * müssen einmalig von main an performConnection übergeben werden, damit er dort verfügbar ist. */
void setUpShmemPointers(struct gameInfo *pGeneral, struct playerInfo *pPlayers) {
    pGeneralInfo = pGeneral;
    pPlayerInfo = pPlayers;
}






