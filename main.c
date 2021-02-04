#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

#include "shmfunctions.h"
#include "mainloop.h"
#include "thinkerfunctions.h"
#include "config.h"

#define IP_BUFFER 256 // für Array mit IP-Adresse
#define MOVE_BUFFER 256
#define GAME_ID_LENGTH 13

static int fd[2];
static bool sigFlagMoves = false;
static int shmidGeneralInfo = -1;
static int shmidPlayerInfo = -1;

/* Speichert die IP-Adresse von Hostname in punktierter Darstellung in einem char Array ab.
 * Gibt -1 im Fehlerfall zurück, sonst 0. */
int hostnameToIp(char *ipA, char *hostname) {
    struct in_addr **addr_list;
    struct hostent *host;

    host = gethostbyname(hostname);
    if (host == NULL) {
        herror("Konnte Rechner nicht finden");
        return -1;
    }

    addr_list = (struct in_addr **) host->h_addr_list;

    for (int i = 0; addr_list[i] != NULL; i++) {
        strcpy(ipA, inet_ntoa(*addr_list[i]));
    }
    return 0;
}

/* Räumt auf: Ruft cleanupThinkerfunctions auf, damit auch dort saubergemacht wird. Entfernt die Shmemory-Segmente für
 * allgemeine Infos und Spielerinfos. Schließt die Pipeenden. */
void cleanupMain(void) {
    if (cleanupThinkerfunctions() == -1) { fprintf(stderr, "Fehler beim Aufräumen im Thinker\n"); }

    if (shmidGeneralInfo != -1) {
        if (shmDelete(shmidGeneralInfo) > 0) { fprintf(stderr, "Fehler beim Aufräumen in main: Konnte shmidGeneralInfo nicht entfernen\n"); }
        else { shmidGeneralInfo = -1; }
    }

    if (shmidPlayerInfo != -1) {
        if (shmDelete(shmidPlayerInfo) > 0) { fprintf(stderr, "Fehler beim Aufräumen in main: Konnte shmidPlayerInfo nicht entfernen\n"); }
        else { shmidPlayerInfo = -1; }
    }

    if (fd[0] != -1) {
        if (close(fd[0]) != 0) { perror("Fehler beim Aufräumen: Konnte Pipeende 0 nicht schließen"); }
        else { fd[0] = -1; }
    }

    if (fd[1] != -1) {
        if (close(fd[1]) != 0) { perror("Fehler beim Aufräumen: Konnte Pipeende 1 nicht schließen"); }
        else { fd[1] = -1; }
    }
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
        perror("Fehler! Der Signal-Handler konnte nicht registriert werden");
        cleanupMain();
        return EXIT_FAILURE;
    }

    // legt Variablen für Game-ID, Spielernummer und Pfad der Konfigurationsdatei an
    char gameID[GAME_ID_LENGTH + 1] = "";
    int wantedPlayerNumber = -1;
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
                if (wantedPlayerNumber <= 0) {
                    // prüft nur, ob Spielernummer 0 (auch durch Nicht-Int als Eingabe) oder negativ ist
                    // alle positiven Ints werden hier durchgelassen -> Erweiterbarkeit auf mehr Spieler
                    fprintf(stderr, "Fehler! Ungültige Spielernummer.\n");
                    printHelp();
                    return EXIT_FAILURE;
                }
                wantedPlayerNumber--; // übergebene Spielernummern sind 1-2, aber der Server will 0-1
                break;
            case 'c':
                confFilePath = optarg;
                break;
            default:
                printHelp();
                return EXIT_FAILURE;
        }
    }

    // erzwingt, dass eine Game-ID angegeben wurde -> bricht sonst Programm ab
    if (strcmp(gameID, "") == 0) {
        fprintf(stderr, "Fehler! Spielernummer nicht angegeben.\n");
        printHelp();
        return EXIT_FAILURE;
    }

    // öffnet confFilePath und schreibt die dortigen Konfigurationswerte in das Struct configInfo
    struct cnfgInfo configInfo;
    if (readFromConfFile(&configInfo, confFilePath) == -1) {
        fprintf(stderr, "Fehler beim Lesen aus der Konfigurationsdatei.\n");
        return EXIT_FAILURE;
    }

    // erzeugt ein struct GameInfo in einem dafür angelegten Shared-Memory-Bereich
    shmidGeneralInfo = shmCreate(sizeof(struct gameInfo));
    if (shmidGeneralInfo == -1) {
        fprintf(stderr, "Fehler! Shared Memory für allgemeine Informationen konnte nicht erstellt werden.\n");
        cleanupMain();
        return EXIT_FAILURE;
    }
    struct gameInfo *pGeneralInfo = shmAttach(shmidGeneralInfo);
    if (pGeneralInfo == NULL) {
        fprintf(stderr, "Fehler! Shared Memory für allgemeine Informationen konnte nicht angebunden werden.\n");
        cleanupMain();
        return EXIT_FAILURE;
    }
    *pGeneralInfo = createGameInfoStruct();

    // legt einen Shared-Memory-Bereich für die struct playerInfos an
    shmidPlayerInfo = createShmemoryForPlayers();
    if (shmidPlayerInfo == -1) {
        fprintf(stderr, "Fehler! Shared Memory für Spielerinformationen konnte nicht erstellt werden.\n");
        cleanupMain();
        return EXIT_FAILURE;
    }
    struct playerInfo *pPlayerInfo = shmAttach(shmidPlayerInfo);
    if (pPlayerInfo == NULL) {
        fprintf(stderr, "Fehler! Shared Memory für Spielerinformationen konnte nicht angebunden werden.\n");
        cleanupMain();
        return EXIT_FAILURE;
    }
    setUpPlayerAlloc(pPlayerInfo);
    setUpShmemPointers(pGeneralInfo, pPlayerInfo);

    // Erstellung der Pipe
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

        // schließt Schreibende der Pipe
        if (close(fd[1]) != 0) { perror("Fehler beim Schließen von Pipeende 1"); } // kein kritischer Fehler -> weitermachen
        fd[1] = -1;

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

        mainloop_epoll(sock, fd[0], gameID, wantedPlayerNumber);

        close(sock);

    } else {
        // wir sind im Elternprozess -> Thinker

        // schließt Leseende der Pipe
        if (close(fd[0]) != 0) { perror("Fehler beim Schließen von Pipeende 0"); } // kein kritischer Fehler -> weitermachen
        fd[0] = -1;

        // schreibt die Thinker-PID in das Struct mit den gemeinsamen Spielinformationen
        pGeneralInfo->pidThinker = getpid();

        // Handler registrieren
        if (signal(SIGUSR1, sigHandlerMoves) == SIG_ERR) {
            perror( "Fehler! Der Signal-Handler konnte nicht registriert werden");
            cleanupMain();
            return EXIT_FAILURE;
        }

	do {
        	pause();
	} while(!pGeneralInfo->newMoveInfoAvailable);

        if (!pGeneralInfo->isActive && !pGeneralInfo->prologueSuccessful) {
            printf("Es gab im Connector einen Fehler schon während des Prologs. Der Thinker beendet sich jetzt auch.\n");
            cleanupMain();
            return EXIT_SUCCESS;
        }

        if (setUpMemoryForThinker(pGeneralInfo)) {
            fprintf(stderr, "Fehler bei der bei der Vorbereitung der Speicherbereiche für den Thinker (genauere Infos in der Fehlermeldung eins weiter oben).\n");
            cleanupMain();
            return EXIT_FAILURE;
        }

        char moveString[MOVE_BUFFER] = "";

        while (pGeneralInfo->isActive) {
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

        printf("Die letzte Stellung war:\n");

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
        cleanupMain();
        wait(NULL);
    }

    return EXIT_SUCCESS;
}
