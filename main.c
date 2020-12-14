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

#include "shmfunctions.h"
#include "performConnection.h"
#include "config.h"

#define IP_BUF 256 // für Array mit IP-Adresse
#define PIPE_BUF 256
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
    char wantedGameID[14];
    int playerNumber = 0;
    char *confFilePath = "client.conf";

    // liest Werte für die eben angelegten Variablen von der Kommandozeile ein
    int input;
    while ((input = getopt(argc, argv, "g:p:c:")) != -1) {
        switch (input) {
            case 'g':
                 strncpy(wantedGameID, optarg, 13);
                 wantedGameID[13] = '\0';
                break;
            case 'p':
                playerNumber = atoi(optarg);
                break;
            case 'c':
                confFilePath = optarg;
                break;
            default:
                printHelp();
                return EXIT_FAILURE;
        }
    }

    // überprüft die übergebenen Parameter auf Gültigkeit: wantedGameID nicht leer, playerNumber > 0
    if (strcmp(wantedGameID, "") == 0) {
        fprintf(stderr, "Fehler! Spielernummer nicht angegeben oder ungültig.\n");
        printHelp();
        return EXIT_FAILURE;
    }
    if (playerNumber < 0) {
        fprintf(stderr, "Fehler! Ungültige Spielernummer.\n");
        printHelp();
        return EXIT_FAILURE;
    }
    playerNumber--; // übergebene Spielernummern sind 1-2, aber der Server will 0-1

//    // für mich zur Kontrolle, gehört später weg
//    printf("Folgende Werte wurden übergeben:\n");
//    printf("Gewünschte Game-ID: %s\n", wantedGameID);
//    printf("Spielernummer: %d\n", playerNumber);
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
    char pipeBuffer[PIPE_BUF];

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
        char ip[IP_BUF]; // hier wird die IP-Adresse (in punktierter Darstellung) gespeichert
        hostnameToIp(ip, configInfo.hostName);

        // Socket erstellen
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("\n Error: Socket konnte nicht erstellt werden \n");
        }

        inet_aton(ip, &address.sin_addr);
        if (connect(sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
            printf("\n Error: Connect schiefgelaufen \n");
        }

        performConnection(sock, wantedGameID, playerNumber, pGeneralInfo, pPlayerInfo, MAX_NUMBER_OF_PLAYERS);

        close(sock);

    } else {
        // wir sind im Elternprozess -> Thinker
        close(fd[0]); // schließt Leseende der Pipe

        // schreibt die Thinker-PID in das Struct mit den gemeinsamen Spielinformationen
        pGeneralInfo->pidThinker = getpid();

        wait(NULL);

        // nur zum Testen, ob die Informationen richtig aus dem Shmemory-Bereich gelesen werden können
        printf("Wir spielen die Partie %s (%s) mit %d Spielern.\n", pGeneralInfo->gameName, pGeneralInfo->gameKindName, pGeneralInfo->numberOfPlayers);
        printf("Ich bin %s und spiele als Nummer %d.\n", pPlayerInfo->playerName, pPlayerInfo->playerNumber);
        printf("Ich bin %s und spiele als Nummer %d.\n", (pPlayerInfo+1)->playerName, (pPlayerInfo+1)->playerNumber);


        // Shared-Memory-Bereiche aufräumen
        if (shmDelete(shmidGeneralInfo) > 0) return EXIT_FAILURE;
        if (shmDelete(shmidPlayerInfo) > 0) return EXIT_FAILURE;

    }

    return EXIT_SUCCESS;

}
