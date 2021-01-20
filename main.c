#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <signal.h>

#include "shmfunctions.h"
#include "mainloop.h"
#include "thinkerfunctions.h"
#include "config.h"

#define IP_BUFFER 256 // für Array mit IP-Adresse
#define MOVE_BUFFER 256
#define GAME_ID_LENGTH 13

static bool sigFlagMoves = false;
static int shmidGeneralInfo = -1;
static int shmidPlayerInfo = -1;

/* Speichert die IP-Adresse von Hostname in punktierter Darstellung in einem char Array ab.
 * Gibt -1 im Fehlerfall zurück, sonst 0. */
int hostnameToIp(char *ipA, char *hostname) {
    struct in_addr **addr_list;
    struct hostent *host;

    host = gethostbyname(hostname);
    if (host==NULL){
        herror("Konnte Rechner nicht finden");
	return -1;
    }

    addr_list = (struct in_addr **) host->h_addr_list;

    for (int i = 0; addr_list[i] != NULL; i++) {
        strcpy(ipA, inet_ntoa(*addr_list[i]));
    }
    return 0;
}

/* Räumt auf: gemallocten Speicher freigeben, Shmemory-Segmente löschen.
 * Ruft auch cleanupThinkerfunctions auf, sodass auch dort saubergemacht wird.
 * Im Fehlerfall wird nicht sofort -1 zurückgegeben, sondern erst am Ende der Funktion, weil versucht werden soll,
 * zumindest noch andere Stellen aufzuräumen. */
int cleanupMain(void) {
    int ret = 0;

    if (cleanupThinkerfunctions() == -1) ret = -1;

    if (shmidGeneralInfo != -1) {
        if (shmDelete(shmidGeneralInfo) > 0) ret = -1;
    }

    if (shmidPlayerInfo != -1) {
        if (shmDelete(shmidPlayerInfo) > 0) ret = -1;
    }

    return ret;
}

// signal handler Ctrl-C
void sigHandlerCtrlC(int sig_nr) {
    printf("\nProgramm wurde mit ctrl-c (Signal = %d) beendet. \n", sig_nr);
    cleanupMain();
    exit(EXIT_SUCCESS);
}

// signal handler Thinker
void sigHandlerMoves(int sig_nr) {
    printf("Thinker: Signal (%d) bekommen vom Connector. (Moves) \n", sig_nr);
    sigFlagMoves = true;
}

int main(int argc, char *argv[]) {

	// Signalhandler der Main Methode registrieren
	if (signal(SIGINT, sigHandlerCtrlC) == SIG_ERR) {
		fprintf(stderr, "Fehler! Der Signal-Handler konnte nicht registriert werden.\n");
		cleanupMain();
		return EXIT_FAILURE;
	}

    // legt Variablen für Game-ID, Spielernummer und Pfad der Konfigurationsdatei an
    char gameID[GAME_ID_LENGTH+1] = "";
    int wantedPlayerNumber = 0;
    char *confFilePath = "client.conf";

    // liest Werte für die eben angelegten Variablen von der Kommandozeile ein
    int input;
    while ((input = getopt(argc, argv, "g:p:c:")) != -1) {
        switch (input) {
            case 'g':
                strncpy(gameID, optarg, GAME_ID_LENGTH);
                gameID[GAME_ID_LENGTH] = '\0';
                break;
            case 'p':
                wantedPlayerNumber = atoi(optarg);
                break;
            case 'c':
                confFilePath = optarg;
                break;
            default:
                printHelp();
                return EXIT_FAILURE;
        }
    }

    // überprüft die übergebenen Parameter auf Gültigkeit: gameID nicht leer, wantedPayerNumber > 0
    if (strcmp(gameID, "") == 0) {
        fprintf(stderr, "Fehler! Spielernummer nicht angegeben oder ungültig.\n");
        printHelp();
        return EXIT_FAILURE;
    }
    if (wantedPlayerNumber < 0) {
        fprintf(stderr, "Fehler! Ungültige Spielernummer.\n");
        printHelp();
        return EXIT_FAILURE;
    }
    wantedPlayerNumber--; // übergebene Spielernummern sind 1-2, aber der Server will 0-1

    // öffnet confFilePath und schreibt die dortigen Konfigurationswerte in das Struct configInfo
    struct cnfgInfo configInfo;
    if (readFromConfFile(&configInfo, confFilePath) == -1) return EXIT_FAILURE;

    // erzeugt ein struct GameInfo in einem dafür angelegten Shared-Memory-Bereich
    shmidGeneralInfo = shmCreate(sizeof(struct gameInfo));
    struct gameInfo *pGeneralInfo = shmAttach(shmidGeneralInfo);

    *pGeneralInfo = createGameInfoStruct();

    // legt einen Shared-Memory-Bereich für die struct playerInfos an
    shmidPlayerInfo = createShmemoryForPlayers();
    struct playerInfo *pPlayerInfo = shmAttach(shmidPlayerInfo);
    setUpPlayerAlloc(pPlayerInfo);
    setUpShmemPointers(pGeneralInfo, pPlayerInfo);

    // Erstellung der Pipe
    int fd[2];

    if (pipe(fd) < 0) {
        perror("Fehler beim Erstellen der Pipe");
        cleanupMain();
        return EXIT_FAILURE;
    }

    // Aufteilung in zwei Prozesse
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fehler beim Forking");
        cleanupMain();
        return EXIT_FAILURE;

    } else if (pid == 0) {
        // wir sind im Kindprozess -> Connector
        close(fd[1]); // schließt Schreibende der Pipe

        // schreibt die Connector-PID in das Struct mit den gemeinsamen Spielinformationen
        pGeneralInfo->pidConnector = getpid();

        // Socket vorbereiten
        int sock = 0;
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_port = htons(configInfo.portNumber);
        char ip[IP_BUFFER]; // hier wird die IP-Adresse (in punktierter Darstellung) gespeichert
        if (hostnameToIp(ip, configInfo.hostName) == -1) {
            cleanupMain();
            return EXIT_FAILURE;
	    }

        // Socket erstellen
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            fprintf(stderr, "Fehler! Socket konnte nicht erstellt werden. \n");
            cleanupMain();
            return EXIT_FAILURE;
        }

        inet_aton(ip, &address.sin_addr);
        if (connect(sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
            fprintf(stderr, "Fehler! Connect schiefgelaufen. \n");
            cleanupMain();
            close(sock);
	        return EXIT_FAILURE;
        }

        mainloop_epoll(sock, fd, gameID, wantedPlayerNumber);

        close(sock);

    } else {
        // wir sind im Elternprozess -> Thinker
        close(fd[0]); // schließt Leseende der Pipe

        // schreibt die Thinker-PID in das Struct mit den gemeinsamen Spielinformationen
        pGeneralInfo->pidThinker = getpid();

        // Handler registrieren
    	if (signal(SIGUSR1, sigHandlerMoves) == SIG_ERR) {
    	    fprintf(stderr, "Fehler! Der Signal-Handler konnte nicht registriert werden.\n");
    	    cleanupMain();
    	    return EXIT_FAILURE;
    	}

        pause();

        if (!pGeneralInfo->isActive) { // wenn das Spiel bereits inaktiv ist (durch Fehler im Prolog im Connector), soll sich auch der Thinker beenden
            printf("Das Spiel ist nicht mehr aktiv. Entweder es gab im Connector einen Fehler oder die Partie wurde bereits zu Ende gespielt. Der Thinker beendet sich jetzt auch.\n");
            cleanupMain();
            return EXIT_SUCCESS;
        }

        if (setUpMemoryForThinker(pGeneralInfo)) {
            fprintf(stderr, "Fehler bei der bei der Vorbereitung der Speicherbereiche für den Thinker (genauere Infos in der Fehlermeldung eins weiter oben).\n");
            cleanupMain();
            return EXIT_FAILURE;
        }

        char moveString[MOVE_BUFFER] = "";

        while(pGeneralInfo->isActive){
            if (sigFlagMoves && pGeneralInfo->newMoveInfoAvailable) {
                pGeneralInfo->newMoveInfoAvailable = false;
                sigFlagMoves = false;

                prepareNewRound();
                memset(moveString, 0, strlen(moveString));

                if (placePiecesOnBoard() != 0) {
                    fprintf(stderr, "Fehler beim Setzen der Steine auf das Spielbrett während des Spiels\n");
                    cleanupMain();
                    return EXIT_FAILURE;
                }

                printf("\n=========NEUE RUNDE=========\n");
                printFull();

                think(moveString);
                printf("\nDer beste Zug ist: %s\n", moveString);
                write(fd[1], moveString, strlen(moveString));
            } else if (sigFlagMoves) { // wenn nur sigFlagMoves, ohne dass es neue Infos gibt (d.h. wenn einfach so ein SIGUSR1 gekommen ist)
                sigFlagMoves = false; // Flag zurücksetzen und wieder auf Signal warten
            }
            pause();
        }

        printf("Das Spiel ist vorbei. Die letzte Stellung war:\n");

        prepareNewRound();
        printf("\n=========LETZTE STELLUNG=========\n");
        if (placePiecesOnBoard() != 0) {
            fprintf(stderr, "Fehler beim Setzen der Steine auf das Spielbrett nach dem Game Over\n");
            cleanupMain();
            return EXIT_FAILURE;
        }
        printFull();

        printf("Der Thinker beendet sich jetzt auch.\n");

        // Aufräumarbeiten
        if (cleanupMain() != 0) fprintf(stderr, "Fehler beim Aufräumen\n");
        wait(NULL);
    }

    return EXIT_SUCCESS;
}
