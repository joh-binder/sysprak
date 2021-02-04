#include <sys/epoll.h>
#include <signal.h>

#include "mainloop.h"
#include "util.h"

#define MAX_LEN 1024
#define MAX_EVENTS 2
#define CLIENTVERSION "2.0"
#define GAMEKINDNAME "Bashni"
#define GAME_ID_LENGTH 13

typedef struct
{
    char buffer[MAX_LEN];
    int filled;
    void (*line)(char*);
} LineBuffer;

typedef enum {
    EXPECT_WELCOME_LINE,
    EXPECT_GAME_ID_PROMPT,
    EXPECT_GAME_KIND_NAME,
    EXPECT_GAME_NAME,
    EXPECT_OWN_PLAYER_INFO,
    EXPECT_TOTAL_PLAYER_INFO,
    EXPECT_OPP_PLAYER_INFO,
    MAIN_STATE,
    MOVE,
    GAME_OVER,
    READING_PIECES_FIRST_TIME,
    READING_PIECES_REGULAR
} protocol_state;

int sockfiled = -1;
int pipefiled = -1;
char gameID[GAME_ID_LENGTH + 1];
int wantedPlayerNumber;

//Epoll struct um Server und Pipe gleichzeitig auf eingehende Nachrichten zu überprüfen
struct epoll_event eventpipe, eventsock, events[MAX_EVENTS];
int epollfd, nfds;

protocol_state current_state = EXPECT_WELCOME_LINE;

// Pointer auf die Shared-Memory-Bereiche
static struct gameInfo *pGeneralInfo;
static struct playerInfo *pPlayerInfo;
static struct line *pMoves;

// hier kommen die Infos zu den Gegnern rein
static struct tempPlayerInfo oppInfo[MAX_NUMBER_OF_PLAYERS_IN_SHMEM-1];
int countOpponents = 0;

int countPieceLines = 0;
struct line *pTempMemoryForPieces = NULL;

static bool moveShmExists = false;
static protocol_state prevState; // Rücksprungzustand nach dem Einlesen von Steinen

static char gamekindserver[MAX_LEN];
static char gamename[MAX_LEN];
static char playerName[MAX_LEN];
static int totalplayer;
static int ownPlayerNumber;

void mainloop_cleanup(void) {
    pGeneralInfo->isActive = false;
    if (sockfiled != -1) close(sockfiled);
    if (pipefiled != -1) close(pipefiled);

    if (pTempMemoryForPieces != NULL) {
        free(pTempMemoryForPieces);
        pTempMemoryForPieces = NULL;
    }

    // sendet ein Signal an Thinker, damit er am pause vorbeikommt und sich auch beenden kann
    if (kill(pGeneralInfo->pidThinker, SIGUSR1) != 0) {
        fprintf(stderr, "Fehler beim Senden des Signals in mainloop_cleanup.\n");
    }
    printf("Der Connector beendet sich jetzt.\n");

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




void mainloop_pipeline(char* line) {

    char buffer[MAX_LEN];

    memset(buffer, 0, MAX_LEN);
    snprintf(buffer, MAX_LEN, "PLAY %s\n", line);
    //write(sockfiled, buffer, strlen(buffer));
    ownWrite(sockfiled, buffer);

}




void mainloop_sockline(char* line) {

    char buffer[MAX_LEN];

    // Variablen, die hier standen, sind jetzt ganz oben statisch deklariert
    // valgrind ist nämlich glücklicher, wenn die initialisiert werden, und bei statischen Variablen passiert das von selbst

    strcpy(buffer,"");

    if (strlen(line) > 1 && *line == '-') {

        // Fehlerbehandlung bei falschen Angaben
        if (strcmp(line + 2, "TIMEOUT Be faster next time") == 0) {
            fprintf(stderr, "Fehler! Zu langsam. Versuche beim nächsten Mal schneller zu sein. (Server: %s)\n", line + 2);
        } else if(strcmp(line + 2, "Not a valid game ID") == 0) {
            fprintf(stderr, "Fehler! Keine zulässige Game ID. (Server: %s)\n", line + 2);
        } else if(strcmp(line + 2, "Game does not exist") == 0) {
            fprintf(stderr, "Fehler! Dieses Spiel existiert nicht. (Server: %s)\n", line + 2);
        } else if(strcmp(line + 2, "No free player") == 0) {
            fprintf(stderr, "Fehler! Spieler ist nicht verfügbar. (Server: %s)\n", line + 2);
        } else if(strcmp(line + 2, "Client Version does not match server Version") == 0) {
            fprintf(stderr, "Fehler! Die Client Version und Server Version stimmen nicht überein. (Server: %s)\n", line + 2);
        } else if(startsWith(line+2, "Invalid Move")) {
            fprintf(stderr, "Fehler! Es wurde ein ungültiger Spielzug übermittelt. (Server: %s)\n", line + 2);
        } else {
            fprintf(stderr, "Es ist ein unerwarteter Fehler aufgetreten. (Server: %s)\n", line + 2);
        }

        mainloop_cleanup();
        exit(EXIT_FAILURE);
    }

    // Erwartet: + MNM Gameserver v2.3 accepting connections
    if (current_state == EXPECT_WELCOME_LINE) {
        sprintf(buffer, "VERSION %s\n", CLIENTVERSION);

        //write(sockfiled, buffer, strlen(buffer));
        ownWrite(sockfiled, buffer);

        current_state = EXPECT_GAME_ID_PROMPT;



    // Erwartet: + Client version accepted - please send Game-ID to join
    } else if (current_state == EXPECT_GAME_ID_PROMPT) {
        sprintf(buffer, "ID %s\n", gameID);
        //write(sockfiled, buffer, strlen(buffer));
        ownWrite(sockfiled, buffer);
        current_state = EXPECT_GAME_KIND_NAME;


    // Erwartet: + PLAYING Bashni
    } else if (current_state == EXPECT_GAME_KIND_NAME) {
        strncpy(gamekindserver, line + 10, MAX_LEN);
        if (strcmp(gamekindserver, "Bashni") != 0) {
            fprintf(stderr, "Fehler! Falsche Spielart. Spiel des Clients: %s. Spiel des Servers: %s. Client wird beendet.\n", GAMEKINDNAME, gamekindserver);
            mainloop_cleanup();
            exit(EXIT_FAILURE);
        }
        current_state = EXPECT_GAME_NAME;


    // Erwartet: + <<Game-Name>>
    } else if (current_state == EXPECT_GAME_NAME) {
        strncpy(gamename, line+2, MAX_LEN-2);
        gamename[MAX_LEN-1] = '\0';

        if (wantedPlayerNumber == -1) {
            sprintf(buffer, "PLAYER\n");
            //write(sockfiled, buffer, strlen(buffer));
            ownWrite(sockfiled, buffer);

        } else {
            sprintf(buffer, "PLAYER %d\n", wantedPlayerNumber);
            //write(sockfiled, buffer, strlen(buffer));
            ownWrite(sockfiled, buffer);
        }

        // überträgt die allgemeinen Spielinfos in den Shmemory-Bereich
        strncpy(pGeneralInfo->gameKindName, gamekindserver, MAX_LENGTH_NAMES);
        strncpy(pGeneralInfo->gameName, gamename, MAX_LENGTH_NAMES);

        current_state = EXPECT_OWN_PLAYER_INFO;

    // Erwartet: + YOU <<Mitspielernummer>> <<Mitspielername>>
    } else if (current_state == EXPECT_OWN_PLAYER_INFO) {
        if (sscanf(line+6, "%d %[^\n]", &ownPlayerNumber, playerName) != 2) {
            fprintf(stderr, "Fehler beim Verarbeiten der eigenen Spielerinformationen\n");
            mainloop_cleanup();
            exit(EXIT_FAILURE);
        }
        pGeneralInfo->ownPlayerNumber = ownPlayerNumber;

        // überträgt diese Informationen in den Shared-Memory-Bereich
        struct playerInfo *pFirstPlayer = playerShmalloc();
        if (pFirstPlayer == (struct playerInfo *)NULL) {
            fprintf(stderr, "Fehler bei der Zuteilung von Shared Memory für die eigenen Spielerinfos.\n");
            mainloop_cleanup();
            exit(EXIT_FAILURE);
        }
        *pFirstPlayer = createPlayerInfoStruct(ownPlayerNumber, playerName, true);

        current_state = EXPECT_TOTAL_PLAYER_INFO;

    // Erwartet: + TOTAL <<Mitspieleranzahl>>
    } else if (current_state == EXPECT_TOTAL_PLAYER_INFO) {
        printf("Total: %s\n", line + 2);
        if (sscanf(line+2, "%*[^ ] %d", &totalplayer) != 1) {
            fprintf(stderr, "Fehler beim Verarbeiten der Mitspieleranzahl\n");
            mainloop_cleanup();
            exit(EXIT_FAILURE);
        }

        // überprüft, ob für totalplayer Stück Spieler überhaupt genug Shared Memory reserviert wurde
        if (checkPlayerShmallocSize(totalplayer) != 0) {
            fprintf(stderr, "Fehler! Es wird zu wenig Shared-Memory-Platz für %d Spieler bereitgestellt.\n", totalplayer);
            mainloop_cleanup();
            exit(EXIT_FAILURE);
        }

        pGeneralInfo->numberOfPlayers = totalplayer; // diese Information muss im Shmemory noch ergänzt werden

        current_state = EXPECT_OPP_PLAYER_INFO;

    //Erwartet: + <<Mitspielernummer>> <<Mitspielername>> <<Bereit>>
    } else if (current_state == EXPECT_OPP_PLAYER_INFO) {
        if (strcmp(line, "+ ENDPLAYERS") != 0) {

            countOpponents++;
            if (countOpponents >= totalplayer) {
                fprintf(stderr, "Fehler! Mehr Spieler-Infos empfangen als erwartet. Client wird beendet.\n");
                mainloop_cleanup();
                exit(EXIT_FAILURE);
            } else {
                // trennt am Leerzeichen nach Mitspielernummer; Mitspielernummer wird in playerNumber geschrieben; alles dahinter zurück in line
                if (sscanf(line+2, "%d %[^\n]", &oppInfo[countOpponents-1].playerNumber, line) != 2) {
                    fprintf(stderr, "Fehler beim Verarbeiten von Mitspielerinformationen\n");
                    mainloop_cleanup();
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
                    mainloop_cleanup();
                    exit(EXIT_FAILURE);
                }
            }

        } else {
            // es kommen keine oppInfos mehr -> beim nächsten sockline-Aufrufe in den nächsten Zustand gehen
            current_state = MAIN_STATE;

            // jetzt können wir alle Infos ausdrucken
            prettyPrint(gamekindserver, gameID, playerName, totalplayer, oppInfo);

            // außerdem können wir die oppInfos in Shared-Memory übertragen
            for (int i = 0; i < totalplayer-1; i++) {
                struct playerInfo *pNextPlayer = playerShmalloc();
                if (pNextPlayer == (struct playerInfo *)NULL) {
                    fprintf(stderr, "Fehler bei der Zuteilung von Shared Memory für Infos eines anderen Spielers.\n");
                    mainloop_cleanup();
                    exit(EXIT_FAILURE);
                } else {
                    *pNextPlayer = createPlayerInfoStruct(oppInfo[i].playerNumber, oppInfo[i].playerName, oppInfo[i].readyOrNot);
                }
            }

            pGeneralInfo->prologueSuccessful = true;
        }

    } else if (current_state == MAIN_STATE) { // entspricht "normalem" Zustand nach dem Prolog

        if (startsWith(line, "+ WAIT")) {
            // Antwort "OKWAIT" zurückschicken
            sprintf(buffer, "OKWAIT\n");
            //write(sockfiled, buffer, strlen(buffer));
            ownWrite(sockfiled, buffer);
            printf("Ich habe das WAIT beantwortet.\n"); // nur für mich zur Kontrolle, kann weg
        } else if (startsWith(line, "+ MOVE"))  {
            current_state = MOVE;
        } else if (strcmp(line, "+ GAMEOVER") == 0) {
            current_state = GAME_OVER;
        } else {
            fprintf(stderr, "Fehler! Nicht interpretierbare Zeile erhalten: %s\n", line);
            mainloop_cleanup();
            exit(EXIT_FAILURE);
        }

    } else if (current_state == MOVE) { // d.h. Zustand bei Move
        if (startsWith(line, "+ PIECESLIST")) {
            countPieceLines = 0; // auf jeden Fall Zähler für Spielsteine zurücksetzen
            if (!moveShmExists) { // wenn es noch kein Move-Shmemory gibt
                pTempMemoryForPieces = malloc(sizeof(struct line)); // Zwischenspeicher für Steine allozieren lassen
                if (pTempMemoryForPieces == NULL) {
                    fprintf(stderr, "Fehler bei Malloc für die Spielsteine in Runde 1.\n");
                    mainloop_cleanup();
                    exit(EXIT_FAILURE);
                }
                prevState = MOVE;
                current_state = READING_PIECES_FIRST_TIME;
            } else { // wenn es bereits Move-Shmemory gibt
                prevState = MOVE;
                current_state = READING_PIECES_REGULAR;
            }
        } else if (strcmp(line, "+ OKTHINK") == 0) {
            // Signal an Thinker schicken
	        if (kill(pGeneralInfo->pidThinker, SIGUSR1) != 0) {
                fprintf(stderr, "Fehler beim Senden des Signals in Runde 1.\n");
                mainloop_cleanup();
                exit(EXIT_FAILURE);
	        }
        } else if (strcmp(line, "+ MOVEOK") == 0) {
            printf("Spielzug wurde akzeptiert\n");
            current_state = MAIN_STATE;
        } else {
            fprintf(stderr, "Fehler! Unvorhergesehene Nachricht in Zustand Move: %s\n", line);
            mainloop_cleanup();
            exit(EXIT_FAILURE);
        }
    } else if (current_state == GAME_OVER) { // Zustand nach Gameover
        if (startsWith(line, "+ PIECESLIST")) {
            countPieceLines = 0; // Zähler für Spielsteine zurücksetzen
            if (!moveShmExists) { // wenn es noch kein Move-Shmemory gibt (sofort Game Over, z.B. bei Aufruf einer Partie, die schon vorbei ist)
                pTempMemoryForPieces = malloc(sizeof(struct line)); // Zwischenspeicher für die Steine
                if (pTempMemoryForPieces == NULL) {
                    fprintf(stderr, "Fehler bei Malloc für die Spielsteine in Runde 1.\n");
                    mainloop_cleanup();
                    exit(EXIT_FAILURE);
                }
                prevState = GAME_OVER;
                current_state = READING_PIECES_FIRST_TIME;
            } else {
                prevState = GAME_OVER;
                current_state = READING_PIECES_REGULAR;
            }
        } else if (strcmp(line, "+ QUIT") == 0) {
            printf("Das Spiel ist vorbei.\n");
            mainloop_cleanup();
            exit(EXIT_SUCCESS);
        } else if (startsWith(line, "+ PLAYER")) { // erwartet '+ PLAYERXWON Yes/No'
            unsigned int pnum;
            char hasWon[4]; // 4, um "Yes" und Nullbyte aufnehmen zu können
            memset(hasWon, 0, 4);

            if (sscanf(line+8, "%d %*s %s", &pnum, hasWon) != 2) {
                fprintf(stderr, "Fehler beim Interpretieren der Informationen, welcher Spieler gewonnen hat.\n");
                mainloop_cleanup();
                exit(EXIT_SUCCESS);
            }
            if (pnum >= pGeneralInfo->numberOfPlayers) {
                fprintf(stderr, "Fehler! Informationen zu einem Spieler Nr. %d erhalten, aber es gibt nur %d Spieler.\n", pnum, pGeneralInfo->numberOfPlayers);
                mainloop_cleanup();
                exit(EXIT_SUCCESS);
            }

            if (pnum == (unsigned int) pGeneralInfo->ownPlayerNumber) {
                printf("Ich (Spieler %d) habe %s.\n", pnum, (strcmp(hasWon, "Yes") == 0) ? "gewonnen" : "verloren");
            } else {
                struct playerInfo *pThisPlayer = getPlayerFromNumber(pnum, pGeneralInfo->numberOfPlayers);
                if (pThisPlayer == NULL) {
                    fprintf(stderr, "Fehler! Konnte für Spieler Nr. %d keine Informationen im Shared Memory finden.\n", pnum);
                    mainloop_cleanup();
                    exit(EXIT_SUCCESS);
                }
                printf("%s (Spieler %d) hat %s.\n", pThisPlayer->playerName, pnum, (strcmp(hasWon, "Yes") == 0) ? "gewonnen" : "verloren");
            }
        } else {
            fprintf(stderr, "Fehler! Unvorhergesehene Nachricht in Zustand Game Over: %s\n", line);
            mainloop_cleanup();
            exit(EXIT_FAILURE);
        }
    } else if (current_state == READING_PIECES_FIRST_TIME) { // Zustand, in dem zum ersten Mal Steine eingelesen werden --> in Zwischenspeicher, danach in Shmem umschreiben
        if (strcmp(line, "+ ENDPIECESLIST") == 0) {
            pGeneralInfo->sizeMoveShmem = countPieceLines; // Anzahl der Spielsteininfos im Shmemory aktualisieren

            // erzeugt einen Shared-Memory-Bereich in passender Größe für alle Spielsteine (als struct line)
            int shmidMoveInfo = createShmemoryForMoves(countPieceLines);
            pMoves = shmAttach(shmidMoveInfo);
            moveShmExists = true;

            // überträgt alle Spielsteininfos vom gemallocten Zwischenspeicher in das neue Shmemory
            for (int i = 0; i < countPieceLines; i++) {
                pMoves[i] = createLineStruct(pTempMemoryForPieces[i].line);
            }

            pGeneralInfo->newMoveInfoAvailable = true;
            free(pTempMemoryForPieces);
            pTempMemoryForPieces = NULL;

            if (prevState != GAME_OVER) { // bei Game Over wollen wir kein "THINKING" zurückschreiben
                sprintf(buffer, "THINKING\n");
                //write(sockfiled, buffer, strlen(buffer));
                ownWrite(sockfiled, buffer);
            }

            current_state = prevState; // zurück zu Move oder Game Over
        } else { // d.h. diese Zeile ist ein Spielstein
            // printf("Spielstein (erste Runde): %s\n", line+2);
            pTempMemoryForPieces = (struct line *)realloc(pTempMemoryForPieces, (countPieceLines+1) * sizeof(struct line)); // Platz für ein struct line mehr von realloc holen
            pTempMemoryForPieces[countPieceLines] = createLineStruct(line+2); // den empfangenen String in den gereallocten Speicher schreiben
            countPieceLines++;
        }
    } else if (current_state == READING_PIECES_REGULAR) { // Zustand, in dem Steine eingelesen und direkt im Shmemory gespeichert werden
        if (strcmp(line, "+ ENDPIECESLIST") == 0) {
            pGeneralInfo->sizeMoveShmem = countPieceLines; // der Allgemeinheit halber, ist aber in Bashni immer 32
            pGeneralInfo->newMoveInfoAvailable = true;

            if (prevState != GAME_OVER) { // bei Game Over wollen wir kein "THINKING" zurückschreiben
                sprintf(buffer, "THINKING\n");
                //write(sockfiled, buffer, strlen(buffer));
                ownWrite(sockfiled, buffer);
            }

            current_state = prevState; // zurück zu Move oder Game Over
        } else { // d.h. diese Zeile ist ein Spielstein
            // printf("Spielstein (normal): %s\n", line+2);
            pMoves[countPieceLines] = createLineStruct(line+2);
            countPieceLines++;
        }
    } else {
        fprintf(stderr, "Fehler! Unvorhergesehener Zustand mit Nr. %d.\n", current_state);
        mainloop_cleanup();
        exit(EXIT_FAILURE);
    }

}





//Bekommt einen Buffer und macht Zeilen daraus, welche an den LineHandler weitergegeben werden
void mainloop_filehandler(char* buffer, int len, LineBuffer* linebuffer) {

    memcpy(linebuffer->buffer+linebuffer->filled, buffer, len);
    linebuffer->filled += len;

    int oldi = 0;
    for(int i = 0; i < linebuffer->filled; i++) {
        if(linebuffer->buffer[i] == '\n') {
            linebuffer->buffer[i] = '\0';
            (*(linebuffer->line))(&(linebuffer->buffer[oldi]));
            oldi = i + 1;
        }
    }
    // Kopiert Reststring an den Anfang
    memmove(linebuffer->buffer, &(linebuffer->buffer[oldi]), (linebuffer->filled)-oldi);
    linebuffer->filled = (linebuffer->filled)-oldi;
}






// Epoll event Loop, um Pipe und Socket zu überwachen
void mainloop_epoll(int sockfd, int pipefd, char ID[GAME_ID_LENGTH + 1], int playerNum) {

    char buffersock[MAX_LEN];
    char bufferpipe[MAX_LEN];

    memset(buffersock, 0, MAX_LEN);
    memset(bufferpipe, 0, MAX_LEN);

    sockfiled = sockfd;
    pipefiled = pipefd;
    wantedPlayerNumber = playerNum;
    for (long unsigned int i = 0; i < strlen(ID); i++) {
        gameID[i] = ID[i];
    }

    // Erstellt zwei Instanzen des LineBuffer Structs; eins für die Pipe und eins für den Socket
    LineBuffer sockbuffer;
    LineBuffer pipebuffer;
    pipebuffer.filled = 0;
    sockbuffer.filled = 0;
    pipebuffer.line = &mainloop_pipeline;
    sockbuffer.line = &mainloop_sockline;

    // Erstellen der Epoll Instanz
    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("Fehler bei epoll_create\n");
        mainloop_cleanup();
        exit(EXIT_FAILURE);
    }

    // Daten für die zu überwachende Pipe angeben
    eventpipe.events = EPOLLIN;
    eventpipe.data.fd = pipefiled;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, pipefiled, &eventpipe) == -1) {
        perror("Fehler bei epoll_ctl Pipe\n");
        mainloop_cleanup();
        exit(EXIT_FAILURE);
    }

    // Daten für den zu überwachenden Socket angeben
    eventsock.events = EPOLLIN;
    eventsock.data.fd = sockfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &eventsock) == -1) {
        perror("Fehler bei epoll_ctl Socket\n");
        mainloop_cleanup();
        exit(EXIT_FAILURE);
    }

    // Eventloop
    while (pGeneralInfo->isActive) {
        // Warten auf Event
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("Fehler bei epoll_wait\n");
            mainloop_cleanup();
            exit(EXIT_FAILURE);
        }


        for (int n = 0; n < nfds; ++n) {
            // Lesen aus der Pipe
            if (events[n].data.fd == pipefiled) {
                int bufferlen = read(pipefiled, bufferpipe, MAX_LEN);
		        mainloop_filehandler(bufferpipe, bufferlen, &pipebuffer);
            // Lesen vom Socket
            } else if (events[n].data.fd == sockfiled) {
                int bufferlen = read(sockfd, buffersock, MAX_LEN - sockbuffer.filled);
                mainloop_filehandler(buffersock, bufferlen, &sockbuffer);
                if(MAX_LEN == sockbuffer.filled) {
                    printf("Buffer ist voll und es wurde kein newLine Zeichen gelesen.");
                    mainloop_cleanup();
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    // Fehlerbehandlung, wenn closen für den Epollfd fehlschlägt
    if (close(epollfd)) {
        perror("Fehler bei close(epollfd)\n");
        mainloop_cleanup();
        exit(EXIT_FAILURE);
    }

}

/* Die Pointer auf den Shared-Memory-Bereich für die allgemeinen Spielinfos und die Infos zu den einzelnen Spielern
 * müssen einmalig von main an performConnection übergeben werden, damit er dort verfügbar ist. */
void setUpShmemPointers(struct gameInfo *pGeneral, struct playerInfo *pPlayers) {
    pGeneralInfo = pGeneral;
    pPlayerInfo = pPlayers;
}
