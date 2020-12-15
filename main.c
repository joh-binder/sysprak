#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ipc.h>

#include "shmfunctions.h"
#include "thinkerfunctions.h"
#include "performConnection.h"
#include "config.h"

#define IP_BUFFER 256 // für Array mit IP-Adresse
#define PIPE_BUFFER 256
#define MAX_NUMBER_OF_PLAYERS 10


// speichert die IP-Adresse von Hostname in punktierter Darstellung in einem char Array ab
void hostnameToIp(char *ipA, char *hostname) {
    struct in_addr **addr_list;
    struct hostent *host;

    host = gethostbyname(hostname);
    if (host==NULL){
        herror("Konnte Rechner nicht finden");
    }

    addr_list = (struct in_addr **) host->h_addr_list;

    for (int i = 0; addr_list[i] != NULL; i++) {
        strcpy(ipA, inet_ntoa(*addr_list[i]));
    }
}


int main(int argc, char *argv[]) {

    // legt Variablen für Game-ID, Spielernummer und Pfad der Konfigurationsdatei an
    char gameID[14];
    int wantedPlayerNumber = 0;
    char *confFilePath = "client.conf";

    // liest Werte für die eben angelegten Variablen von der Kommandozeile ein
    int input;
    while ((input = getopt(argc, argv, "g:p:c:")) != -1) {
        switch (input) {
            case 'g':
                strncpy(gameID, optarg, 13);
                gameID[13] = '\0';
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

//    // für mich zur Kontrolle, gehört später weg
//    printf("Folgende Werte wurden übergeben:\n");
//    printf("Game-ID: %s\n", gameID);
//    printf("Gewünschte Spielernummer: %d\n", wantedPlayerNumber);
//    printf("Pfad: %s\n", confFilePath);


    // öffnet confFilePath und schreibt die dortigen Konfigurationswerte in das Struct configInfo
    struct cnfgInfo configInfo;
    if (readFromConfFile(&configInfo, confFilePath) == -1) return EXIT_FAILURE;

//    // für mich zur Kontrolle, gehört später weg
//    printf("Ich habe die Funktion readFromConfFile aufgerufen und die Werte, die in configInfo stehen, sind jetzt:\n");
//    printf("gameKindName: %s\n", configInfo.gameKindName);
//    printf("hostName: %s\n", configInfo.hostName);
//    printf("portNumber: %d\n", configInfo.portNumber);

    // erzeugt ein struct GameInfo in einem dafür angelegten Shared-Memory-Bereich
    int shmidGeneralInfo = shmCreate(sizeof(struct gameInfo));
    struct gameInfo *pGeneralInfo = shmAttach(shmidGeneralInfo);

    // legt einen Shared-Memory-Bereich für die struct playerInfos an
    int shmidPlayerInfo = shmCreate(MAX_NUMBER_OF_PLAYERS * sizeof(struct playerInfo));
    struct playerInfo *pPlayerInfo = shmAttach(shmidPlayerInfo);

    // Erstellung der Pipe
    int fd[2];
    char pipeBuffer[PIPE_BUFFER];

    if (pipe(fd) < 0) {
        perror("Fehler beim Erstellen der Pipe");
        shmDelete(shmidGeneralInfo);
        shmDelete(shmidPlayerInfo);
        return EXIT_FAILURE;
    }

    // Aufteilung in zwei Prozesse
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fehler beim Forking");
        shmDelete(shmidGeneralInfo);
        shmDelete(shmidPlayerInfo);
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
        hostnameToIp(ip, configInfo.hostName);

        // Socket erstellen
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("\n Error: Socket konnte nicht erstellt werden \n");
        }

        inet_aton(ip, &address.sin_addr);
        if (connect(sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
            printf("\n Error: Connect schiefgelaufen \n");
        }

        performConnection(sock, gameID, wantedPlayerNumber, configInfo.gameKindName, pGeneralInfo, pPlayerInfo,
                          MAX_NUMBER_OF_PLAYERS);

        close(sock);

    } else {

        // wir sind im Elternprozess -> Thinker
        close(fd[0]); // schließt Leseende der Pipe

        // schreibt die Thinker-PID in das Struct mit den gemeinsamen Spielinformationen
        pGeneralInfo->pidThinker = getpid();


        tower **pBoard = malloc(sizeof(tower *) * 64);
        tower *pTowers;

        wait(NULL);

        // Shared-Memory für die Spielzüge aufrufen
        int shmidMoveInfo = shmAccessExisting(ftok("main.c", KEY_FOR_MOVE_SHMEM),
                                              pGeneralInfo->sizeMoveShmem * sizeof(struct line));
        struct line *pMoveInfo = shmAttach(shmidMoveInfo);

        // genug Speicherplatz für alle Spielsteine freigeben
        pTowers = malloc(sizeof(tower) * pGeneralInfo->sizeMoveShmem);



        // ab hier: jede Runde wiederholen

        resetTallocCounter();

        // setzt alle Pointer auf dem Brett auf NULL
        for (int i = 0; i < 64; i++) {
            pBoard[i] = NULL;
        }

        printf("Ich lese jetzt vom Thinker aus dem Shmemory-Bereich:\n");
        for (int i = 0; i < pGeneralInfo->sizeMoveShmem; i++) {
            addToSquare(pBoard, codeToCoord(pMoveInfo[i].line+2), pMoveInfo[i].line[0], pTowers, pGeneralInfo->sizeMoveShmem);
        }

        printFull(pBoard);

//        // ein paar Züge zur Probe (entsprechen nicht den Bashni-Regeln, sondern einfach von irgendwo woandershin)
//        moveTower(pBoard, codeToCoord("A3"), codeToCoord("C5"));
//        beatTower(pBoard, codeToCoord("D6"), codeToCoord("C5"), codeToCoord("B4"));
//        beatTower(pBoard, codeToCoord("B4"), codeToCoord("A1"), codeToCoord("B5"));
//        beatTower(pBoard, codeToCoord("C1"), codeToCoord("B5"), codeToCoord("H4"));
//
//        printTopPieces(pBoard);


//        // nur zum Testen, ob die Informationen richtig aus dem Shmemory-Bereich gelesen werden können
//        printf("Wir spielen die Partie %s des Spiels %s mit %d Spielern.\n", pGeneralInfo->gameName, pGeneralInfo->gameKindName, pGeneralInfo->numberOfPlayers);
//        printf("Ich bin %s und spiele als Nummer %d.\n", pPlayerInfo->playerName, pPlayerInfo->playerNumber);
//        printf("Der Gegner ist %s und spielt als Nummer %d.\n", (pPlayerInfo+1)->playerName, (pPlayerInfo+1)->playerNumber);


        // Speicherplatz freigaben
        if (pBoard != NULL) {
            free(pBoard);
        }
        if (pTowers != NULL) {
            free(pTowers);
        }

        // Shared-Memory-Bereiche aufräumen
        if (shmDelete(shmidGeneralInfo) > 0) return EXIT_FAILURE;
        if (shmDelete(shmidPlayerInfo) > 0) return EXIT_FAILURE;
        if (shmDelete(shmidMoveInfo) > 0) return EXIT_FAILURE;

    }

    return EXIT_SUCCESS;
}