#include <stdio.h>
#include <string.h>
#include "config.h"
#include <stdlib.h>

#define CONFIG_BUFFER_SIZE 256

void printHelp(void) {
    printf("Um das Programm auszuführen, müssen Sie folgende Informationen als Kommandozeilenparameter übergeben:\n");
    printf("-g <gameid>: eine 13-stellige Game-ID\n");
    printf("-p <playerno>: Ihre gewünschte Spielernummer (1 oder 2)\n");
    printf("(optional) -c <path>: Dateipfad zu einer Datei mit Konfigurationsinformationen\n");
}

struct cnfgInfo createConfigStruct(void) {
    struct cnfgInfo ret;
    strcpy(ret.gameKindName, "");
    ret.portNumber = 0;
    strcpy(ret.hostName, "");
    return ret;
}

int readFromConfFile(struct cnfgInfo *pConfigInfo, char *path) {

    FILE *datei = fopen(path, "r");
    if (datei == NULL) {
        perror("Fehler beim Öffnen der Konfigurationdatei");
        return -1;
    }

    int countLine = 0;
    char line[CONFIG_BUFFER_SIZE];
    while(fgets(line, CONFIG_BUFFER_SIZE, datei) != NULL) {
        countLine++;

        // wir wollen Leerzeichen und Tabs am Anfang einer Zeile ignorieren -> Pointer einfach weitersetzen
        char *lineAsPointer = line;
        while (*lineAsPointer == ' ' || *lineAsPointer == '\t') lineAsPointer++;

        char varName[CONFIG_BUFFER_SIZE], varValue[CONFIG_BUFFER_SIZE];

        if(sscanf(lineAsPointer, "%[^ \t] = %[^ \t\n]", varName, varValue) != 2) {
            fprintf(stderr, "Fehler beim Einlesen der Werte aus der Konfigurationsdatei in Zeile %d.\n", countLine);
            continue;
        }

        if (strcmp(varName, "hostName") == 0) {
            strcpy(pConfigInfo->hostName, varValue);
        }
        if (strcmp(varName, "gameKindName") == 0) {
            strcpy(pConfigInfo->gameKindName, varValue);
        }
        if (strcmp(varName, "portNumber") == 0) {
            pConfigInfo->portNumber = atoi(varValue);
        }

    }

    fclose(datei);
    return 0;
}