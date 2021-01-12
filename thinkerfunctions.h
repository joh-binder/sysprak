#ifndef SYSPRAK_THINKERFUNCTIONS_H
#define SYSPRAK_THINKERFUNCTIONS_H

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

/* Wandelt einen Buchstabe-Zahl-Code in den Datentyp coordinate um.
 * Bei ungültigen Koordinaten (alles außer A-H und 1-8) sind die Koordinaten -1, -1. */
coordinate codeToCoord(char code[2]);

/* In der Main-Methode sollte ein Spielbrett (tower*) gemalloced werden. Der entstehende Pointer muss einmalig mit
 * dieser Funktion an dieses Modul übergeben werden, damit die statische Variable tower **board gesetzt werden kann,
 * die dann die meisten Funktionen im Modul benutzen. */
void setUpBoard(tower **pBoard);

/* In der Main-Methode sollte Speicher für eine bestimmte Anzahl an Türmen gemalloced werden. Der entstehende Pointer
 * sowie die Anzahl der Türme, die in den Speicher passen, müssen einmalig mit dieser Funktion an dieses Modul
 * übergeben werden, damit statische Variablen gesetzt werden können, die dann andere Funktionen benutzen wollen. */
void setUpTowerAlloc(tower *pStart, unsigned int numTowers);

int setUpWhoIsWho(int playerno);

/* Setzt den Zähler, den towerAlloc verwendet wieder zurück. Zu verwenden, wenn (in einer neuen Iteration) wieder
 * alle Türme auf das Spielbrett gesetzt werden sollen. */
void resetTallocCounter(void);

/* Setzt alle Pointer des Spielbretts auf NULL zurück. */
void resetBoard(void);

/* Wendet printTopPieces an und druckt zusätzlich eine Liste mit allen weißen und schwarzen Türmen.*/
void printFull(void);

/* Erzeugt einen neuen tower/Spielstein und setzt ihn auf das Spielbrett. Verlangt dazu als Parameter eine
 * Zielkoordinate und die Art des Spielsteins (als Char). */
int addToSquare(coordinate c, char piece);

/* Bestimmt den günstigen Spielzug auf Basis des aktuellen Spielbretts. Schreibt diesen in den angegebenen String. */
void think(char *answer);

#endif //SYSPRAK_THINKERFUNCTIONS_H
