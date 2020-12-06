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

#define GAMEKINDNAME "Bashni"
#define PORTNUMBER 1357
#define HOSTNAME "sysprak.priv.lab.nm.ifi.lmu.de"
#define BUF 256 // für Array mit IP-Adresse

// Hilfsnachricht zu den geforderten Kommandozeilenparametern
void printHelp(void) {
    printf("Um das Programm auszuführen, übergeben Sie bitte folgende Informationen als Kommandozeilenparameter:\n");
    printf("-g <gameid>: eine 13-stellige Game-ID\n");
    printf("-p <playerno>: Ihre gewünschte Spielernummer (1 oder 2)\n");
}

// speichert die IP-Adresse von Hostname in punktierter Darstellung in einem char Array ab
void hostnameToIp(char *ipA) {
    struct in_addr **addr_list;
    struct hostent *host;

    host = gethostbyname(HOSTNAME);
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
    char gameID[14] = "";
    int playerNumber = 0;
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

    /* überprüft die übergebenen Parameter auf Gültigkeit: gameID nicht leer, playerNumber > 0.
     * (gedacht für direkten Aufruf von ./sysprak-client; für make play gibt es eigenen Hinweis im Makefile) */
    if (strcmp(gameID, "") == 0) {
        fprintf(stderr, "Fehler! Spielernummer nicht angegeben oder ungültig.\n");
        printHelp();
        return EXIT_FAILURE;
    }
    if (playerNumber <= 0) {
        fprintf(stderr, "Fehler! Spielernummer nicht angegeben oder ungültig.\n");
        printHelp();
        return EXIT_FAILURE;
    }

    // für mich zur Kontrolle, gehört später weg
    printf("Folgende Werte wurden übergeben:\n");
    printf("Game-ID: %s\n", gameID);
    printf("Spielernummer: %d\n", playerNumber);
    printf("Pfad: %s\n", confFilePath);

    // öffnet confFilePath und schreibt die dortigen Konfigurationswerte in das Struct configInfo
    struct cnfgInfo configInfo = createConfigStruct();
    if (readFromConfFile(&configInfo, confFilePath) == -1) return EXIT_FAILURE;

    // für mich zur Kontrolle, gehört später weg
    printf("Ich habe die Funktion readFromConfFile aufgerufen und die Werte, die in configInfo stehen, sind jetzt:\n");
    printf("gameKindName: %s\n", configInfo.gameKindName);
    printf("hostName: %s\n", configInfo.hostName);
    printf("portNumber: %d\n", configInfo.portNumber);

    // legt einen Shared-Memory-Bereich mit Größe 1000 an
    int shmid = shmCreate(1000);
    void *hierIstShmemory = shmAttach(shmid);

    // Erstellung der Pipe
    int fd[2];
    char pipeBuffer[PIPE_BUF];

    if (pipe(fd) < 0) {
        perror("Fehler beim Erstellen der Pipe");
        return EXIT_FAILURE;
    }

    // Aufteilung in zwei Prozesse
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fehler beim Forking");
        return EXIT_FAILURE;
    } else if (pid == 0) {
        // wir sind im Kindprozess -> Connector
        close(fd[1]); // schließt Schreibende der Pipe
        close(fd[1]); // schließt Schreibende der Pipe

        // Socket vorbereiten
        int sock = 0;
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_port = htons(PORTNUMBER);
        char ip[BUF]; // hier wird die IP-Adresse (in punktierter Darstellung) gespeichert
        hostnameToIp(ip);

        printf("IP lautet: %s\n", ip);

        // Socket erstellen
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            printf("\n Error: Socket konnte nicht erstellt werden \n");
        }

        inet_aton(ip,&address.sin_addr);
        if(connect(sock,(struct sockaddr*) &address, sizeof(address)) < 0) {
            printf("\n Error: Connect schiefgelaufen \n");
        }

        performConnection(sock, gameID);

        close(sock);

        // schreibt zwei Ints in den Shared-Memory-Bereich (auf umständliche Art)
        // (wahrscheinlich würde man einfach eine neue Variable für den gecasteten Pointer anlegen)
        *(int *)hierIstShmemory = 5649754;
        *((int *)hierIstShmemory + 1) = 1122;

    } else {
        // wir sind im Elternprozess -> Thinker
        close(fd[0]); // schließt Leseende der Pipe

        // liest zwei im Kindprozess geschriebene Ints aus dem Shared-Memory-Bereich
        wait(NULL);
        printf("Im Shmemory-Bereich steht: %d\n", *(int *)hierIstShmemory);
        printf("Im Shmemory-Bereich steht auch noch: %d\n", *((int *)hierIstShmemory + 1));

        if (shmDelete(shmid) > 0) return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
