#ifndef SYSPRAK_THINKERFUNCTIONS_H
#define SYSPRAK_THINKERFUNCTIONS_H

#include "shmfunctions.h"
#include "util.h"

typedef struct coord {
    int xCoord;
    int yCoord;
} coordinate;

typedef struct tow {
    char piece;
    struct tow *next;
} tower;

typedef struct moveInfo {
    coordinate origin;
    coordinate target;
    float rating;
} move;

/* In dieser Funktion werden die Speicherbereiche für den Thinker richtig eingerichtet. Die Funktion muss einmal
 * aufgerufen werden, damit der Thinker einsatzbereit ist. Genauer passiert Folgendes:
 *
 * - Der Pointer auf das Struct mit den allgemeinen Informationen wird als Parameter übergeben und für den Thinker als
 *   statische Variable gesetzt.
 * - Es wird festgelegt, ob der Benutzer Spieler 0 oder Spieler 1 ist (dazu ist der Pointer Voraussetzung).
 * - Auf das im Connector angelegte Shmemory mit den Infos zu den Spielsteinen wird zugegriffen.
 * - Für das Spielbrett wird Speicherbereich gemalloced.
 * - Für die Spielsteine wird Speicherbereich gemalloced.
 *
 * Gibt im Normalfall 0 und im Fehlerfall -1 (+ Fehlermeldung zur genaueren Lokalisierung) zurück. */
int setUpMemoryForThinker(struct gameInfo *pG);

// Bündelt die Funktionen resetTallocCounter und resetBoard, da diese sowieso miteinander verwendet werden sollen.
void prepareNewRound(void);

// Druckt Spielbrett mit den obersten Spielsteinen und zusätzlich eine Liste mit allen weißen und schwarzen Türmen.
void printFull(void);

// Liest Spielsteine aus dem Shmemory und setzt sie auf das Spielbrett. Gibt im Fehlerfall -1 zurück, sonst 0.
int placePiecesOnBoard(void);

/* Nimmt einen String und dessen Länge als Parameter. Bestimmt den günstigen Spielzug auf Basis des aktuellen
 * Spielbretts. Schreibt diesen in den angegebenen String, sofern darin genug Platz, und gibt 0 zurück; sonst -1. */
int think(char *answer, int answerMaxLength);

// Räumt auf: gemallocten Speicher freigeben, Shmemory-Segmente löschen.
int cleanupThinkerfunctions(void);

#endif //SYSPRAK_THINKERFUNCTIONS_H
