#include <stdio.h>
#include "config.h"
#include "util.h"

#define CONFIG_BUFFER_SIZE 256

// Hilfsnachricht zu den geforderten Kommandozeilenparametern
void printHelp(void) {
    printf("Um das Programm auszuführen, müssen Sie folgende Informationen als Kommandozeilenparameter übergeben:\n");
    printf("-g <gameid>: eine 13-stellige Game-ID\n");
    printf("(optional) -p <playerno>: Ihre gewünschte Spielernummer (1 oder 2)\n");
    printf("(optional) -c <path>: Dateipfad zu einer Datei mit Konfigurationsinformationen\n");
    printf("Alternativ verwenden Sie den Aufruf \"make play\" und übergeben die Informationen als Umgebungsvariablen.\n");
}


/* nimmt als Parameter ein Pointer auf struct cnfgInfo und einen Dateipfad entgegen;
liest die Datei und schreibt die passenden Informationen in das Struct;
gibt im Fehlerfall -1 zurück, sonst 0 */
int readFromConfFile(struct cnfgInfo *pConfigInfo, char *path) {

    // versucht, Datei an angegebenem Pfad zu öffnen
    FILE *datei = fopen(path, "r");
    if (datei == NULL) {
        perror("Fehler beim Öffnen der Konfigurationdatei");
        return -1;
    }

    int countLine = 0;
    char line[CONFIG_BUFFER_SIZE]; // buffer für einzulesende Zeile
    int readHostName = 0, readGameKindName = 0, readPortNumber = 0; // zur Kontrolle, ob alle Werte eingelesen wurden (0 = False, 1 = True)

    // liest eine Zeile (max. CONFIG_BUFFER_SIZE Zeichen) ein, speichert sie in line
    while(fgets(line, CONFIG_BUFFER_SIZE, datei) != NULL) {
        countLine++;

        // wir wollen Leerzeichen und Tabs am Anfang einer Zeile ignorieren -> Pointer einfach weitersetzen
        char *lineAsPointer = line;
        while (*lineAsPointer == ' ' || *lineAsPointer == '\t') lineAsPointer++;

        // in den Zeilen steht i.d.R. "varName = varValue" -> legt dafür zwei getrennte Variablen an
        char varName[CONFIG_BUFFER_SIZE], varValue[CONFIG_BUFFER_SIZE];

        // scannt bis Leerzeichen oder = nach varName, der Rest landet wieder in lineAsPointer
        sscanf(lineAsPointer, "%[^ \t=]%[^\n]", varName, lineAsPointer);
        while (*lineAsPointer == ' ' || *lineAsPointer == '\t') lineAsPointer++; // evtl. Leerzeichen vor = überspringen
        lineAsPointer++; // = überspringen
        while (*lineAsPointer == ' ' || *lineAsPointer == '\t') lineAsPointer++; // evtl. Leerzeichen nach = überspringen
        strncpy(varValue, lineAsPointer, CONFIG_BUFFER_SIZE-1); // der Rest kann jetzt in varValue

        // je nachdem, was varName ist, soll varValue an eine unterschiedliche Stelle im Struct geschrieben werden
        if (strcmp(varName, "hostName") == 0) {
            strncpy(pConfigInfo->hostName, varValue, MAX_LENGTH_HOSTNAME-1);
            readHostName = 1;
        }
        if (strcmp(varName, "gameKindName") == 0) {
            strncpy(pConfigInfo->gameKindName, varValue, MAX_LENGTH_GAMEKINDNAME-1);
            readGameKindName = 1;
        }
        if (strcmp(varName, "portNumber") == 0) {
            pConfigInfo->portNumber = atoi(varValue);
            readPortNumber = 1;
        }

    }

    // überprüft, ob alle Werte eingelesen wurden
    if (!readHostName || !readGameKindName || !readPortNumber) {
        fprintf(stderr, "Fehler! Mindestens eine der gewünschten Informationen konnte nicht aus der Konfigurationsdatei gelesen werden.\n");
        return -1;
    }

    fclose(datei);
    return 0;
}
