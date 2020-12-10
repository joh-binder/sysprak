#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "shmfunctions.h"
#include "thinkerfunctions.h"

#define GAMEKINDNAME "Bashni"
#define PORTNUMBER 1357
#define HOSTNAME "sysprak.priv.lab.nm.ifi.lmu.de"
#define BUF 256 // für Array mit IP-Adresse


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

//    // legt Variablen für Game-ID und Spielernummer an und liest dann Werte dafür von der Kommandozeile ein
//    char gameID[14] = "";
//    int playerNumber = 0;
//
//    int input;
//    while ((input = getopt(argc, argv, "g:p:")) != -1) {
//        switch (input) {
//            case 'g':
//                strncpy(gameID, optarg, 13);
//                gameID[13] = '\0';
//                break;
//            case 'p':
//                playerNumber = atoi(optarg);
//                break;
//            default:
//                printHelp();
//                break;
//        }
//    }
//
//
//    // legt einen Shared-Memory-Bereich mit Größe 1000 an
//    int shmid = shmCreate(1000);
//    void *hierIstShmemory = shmAttach(shmid);
//
//    // Erstellung der Pipe
//    int fd[2];
//    char pipeBuffer[PIPE_BUF];
//
//    if (pipe(fd) < 0) {
//        perror("Fehler beim Erstellen der Pipe");
//        return EXIT_FAILURE;
//    }

    int boardShmid = shmCreate(sizeof(tower*) * 64);
    tower **boardShmemory = (tower **)shmAttach(boardShmid);

    int towersShmid = shmCreate(sizeof(tower) * (32));
    void *towersShmemory = shmAttach(towersShmid);


    pid_t pid = fork();
    if (pid < 0) {
        perror("Fehler beim Forking");
        return EXIT_FAILURE;

    } else if (pid == 0) {
//        // wir sind im Kindprozess -> Connector
//        close(fd[1]); // schließt Schreibende der Pipe
//
//        // Socket vorbereiten
//        int sock = 0;
//        struct sockaddr_in address;
//        address.sin_family = AF_INET;
//        address.sin_port = htons(PORTNUMBER);
//        char ip[BUF]; // hier wird die IP-Adresse (in punktierter Darstellung) gespeichert
//        hostnameToIp(ip);
//
//        printf("IP lautet: %s\n", ip);
//
//        // Socket erstellen
//        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
//            printf("\n Error: Socket konnte nicht erstellt werden \n");
//        }
//
//        inet_aton(ip,&address.sin_addr);
//        if(connect(sock,(struct sockaddr*) &address, sizeof(address)) < 0) {
//            printf("\n Error: Connect schiefgelaufen \n");
//        }
//
//        // performConnect(sock);
//

        // setzt alle Pointer auf dem Brett auf NULL
        for (int i = 0; i < 64; i++) {
            boardShmemory[i] = NULL;
        }

        // setzt die Startbelegung der Spielsteine auf das Brett
        char *lightSquares[] = {"A1", "C1", "E1", "G1", "B2", "D2", "F2", "H2", "A3", "C3", "E3", "G3"};
        char *darkSquares[] = {"H8", "F8", "D8", "B8", "G7", "E7", "C7", "A7", "H6", "F6", "D6", "B6"};
        for (unsigned long i = 0; i < sizeof(lightSquares)/sizeof(lightSquares[0]); i++) {
            addToSquare(boardShmemory, codeToCoord(lightSquares[i]), 'w', towersShmemory);
        }
        for (unsigned long i = 0; i < sizeof(darkSquares)/sizeof(darkSquares[0]); i++) {
            addToSquare(boardShmemory, codeToCoord(darkSquares[i]), 'b', towersShmemory);
        }

        // ein paar Züge zur Probe (entsprechen nicht den Bashni-Regeln, sondern einfach von irgendwo woandershin)
        moveTower(boardShmemory, codeToCoord("A3"), codeToCoord("C5"));
        beatTower(boardShmemory, codeToCoord("D6"), codeToCoord("C5"), codeToCoord("B4"));
        beatTower(boardShmemory, codeToCoord("B4"), codeToCoord("A1"), codeToCoord("B5"));
        beatTower(boardShmemory, codeToCoord("C1"), codeToCoord("B5"), codeToCoord("H4"));


        //        close(sock);

    } else {

        // druckt Spielfeld aus -> Information kommt im Thinker an
        sleep(1);
        printTopPieces(boardShmemory);

        char *str = malloc(sizeof(char)*33);
        towerToString(str, boardShmemory, codeToCoord("B5"));
        printf("%s\n", str);
        free(str);

        str = malloc(sizeof(char)*33);
        towerToString(str, boardShmemory, codeToCoord("H4"));
        printf("%s\n", str);
        free(str);

//        // wir sind im Elternprozess -> Thinker
//        close(fd[0]); // schließt Leseende der Pipe
//

        wait(NULL);


        if (shmDelete(boardShmid) < 0) return EXIT_FAILURE;
        if (shmDelete(towersShmid) < 0) return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}
