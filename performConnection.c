#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_LEN 1024
#define GAMEKINDNAME "Bashni"
#define PORTNUMBER 1357
#define HOSTNAME "sysprak.priv.lab.nm.ifi.lmu.de"
#define BUF 256 // für Array mit IP-Adresse
#define CLIENTVERSION "2.0"

char recstring[MAX_LEN];
char bufferstring[MAX_LEN];
/*
performConnection benötigt:
    - Game-Name
    - Mitspielername
    - Mitspieleranzahl
    - Bereit
*/

void receive_msg(int sock, char *recmsg){

    int shiftInRecmsg = 0;
    int testrec;

    do {
      memset(bufferstring, 0, strlen(bufferstring));
      testrec = recv(sock, bufferstring, MAX_LEN, 0);

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
    recstring[shiftInRecmsg-1] = '\0';

    //Nur zum Testen eingebaut
    printf("%s\n", recmsg);


    if(strlen(recmsg) > 1 && *recmsg == '-'){

        //Fehlerbehandlung bei falschen Angaben
        if(strcmp(recmsg + 2, "TIMEOUT Be faster next time") == 0){
            fprintf(stderr, "Fehler! Zu langsam. Versuche beim nächsten Mal schnneller zu sein. (Server: %s)\n", recmsg + 2);
        }else if(strcmp(recmsg + 2, "Not a valid game ID") == 0){
            fprintf(stderr, "Fehler! Keine zulässige Game ID. (Server: %s)\n", recmsg + 2);
        }else if(strcmp(recmsg + 2, "Game does not exist") == 0){
            fprintf(stderr, "Fehler! Dieses Spiel existiert nicht. (Server: %s)\n", recmsg + 2);
        }else if(strcmp(recmsg + 2, "No free player") == 0){
            fprintf(stderr, "Fehler! Spieler ist nicht verfügbar. (Server: %s)\n", recmsg + 2);
        }else if(strcmp(recmsg + 2, "Client Version does not match server Version") == 0){
            fprintf(stderr, "Fehler! Die Client Version und Server Version stimmen nicht überein. (Server: %s)\n", recmsg + 2);
        }else{
            fprintf(stderr, "Es ist ein unerwarteter Fehler aufgetreten. (Server: %s)\n", recmsg + 2);
        }

        close(sock);
        exit(EXIT_FAILURE);
    }

}

//Client Nachricht senden
void send_msg(int sock, char *sendmsg){
    int bytesmsg = strlen(sendmsg);
    int bytessend = send(sock, sendmsg, bytesmsg, 0);

    if(bytesmsg != bytessend){
        fprintf(stderr, "Fehler! Bytes der zu sendenden Nachricht: %d, aber gesendete Bytes: %d", bytesmsg, bytessend);
    }
}

void performConnection(int sock, char *gameID, int playerN){

    char gamekindserver[MAX_LEN];

    if(sock <= 0){
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
    strcpy(gamekindserver, recstring + 10);

    //reduziert die Server Nachricht auf "PLAYING " mit dem gamekind
    recstring[16] = '\0';

    if(strcmp(recstring + 2, "PLAYING Bashni") == 0){
        sprintf(buffer, "PLAYER %d\n", playerN);
        send_msg(sock, buffer);
    }else{
        fprintf(stderr, "Fehler! Falsche Spielart. Spiel des Clients: %s. Spiel des Servers: %s. Client wird beendet.\n", GAMEKINDNAME, gamekindserver);
        close(sock);
        exit(EXIT_FAILURE);
    }

    receive_msg(sock, recstring);

}