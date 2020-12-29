#ifndef SYSPRAK_THINKERFUNCTIONS_H
#define SYSPRAK_THINKERFUNCTIONS_H

#include <float.h>
#include "util.h"

/* TODO Kapseln: Viele dieser Funktionen müssen nicht wirklich in main.c sichtbar sein. */

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

/* In der Main-Methode sollte ein Spielbrett (tower*) gemalloced werden. Der entstehende Pointer muss einmalig mit
 * dieser Funktion an dieses Modul übergeben werden, damit die statische Variable tower **board gesetzt werden kann,
 * die dann die meisten Funktionen im Modul benutzen. */
void setUpBoard(tower **pBoard);

/* In der Main-Methode sollte Speicher für eine bestimmte Anzahl an Türmen gemalloced werden. Der entstehende Pointer
 * sowie die Anzahl der Türme, die in den Speicher passen, müssen einmalig mit dieser Funktion an dieses Modul
 * übergeben werden, damit statische Variablen gesetzt werden können, die dann andere Funktionen benutzen wollen. */
void setUpTowerAlloc(tower *pStart, unsigned int numTowers);

int setUpWhoIsWho(int playerno);

/* Setzt voraus, dass ein Speicherblock für Türme reserviert ist. Gibt einen Pointer auf einen Abschnitt des
 * Speicherblocks zurück, der genau groß genug für einen tower ist. Intern wird ein Zähler
 * versetzt, sodass der nächste Funktionsaufruf einen anderen Abschnitt liefert. */
tower *towerAlloc(void);

/* Setzt den Zähler, den towerAlloc verwendet wieder zurück. Zu verwenden, wenn (in einer neuen Iteration) wieder
 * alle Türme auf das Spielbrett gesetzt werden sollen. */
void resetTallocCounter(void);

/* Setzt alle Pointer des Spielbretts auf NULL zurück. */
void resetBoard(void);

// Gibt den zu einer Koordinate auf dem Spielfeld gehörigen Pointer zurück; oder NULL bei ungültiger Koordinate.
tower *getPointerToSquare(coordinate c);

/* Gegeben eine Koordinate, gibt den obersten Spielstein, der auf diesem Feld liegt, als Char zurück;
 * oder 'X' bei ungültiger Koordinate. */
char getTopPiece(coordinate c);

// Druckt zu jedem Feld auf dem Spielbrett den obersten Spielstein formatiert aus.
void printTopPieces(void);

/* Wendet printTopPieces an und druckt zusätzlich eine Liste mit allen weißen und schwarzen Türmen.*/
void printFull(void);

/* Erzeugt einen neuen tower/Spielstein und setzt ihn auf das Spielbrett. Verlangt dazu als Parameter eine
 * Zielkoordinate und die Art des Spielsteins (als Char). */
int addToSquare(coordinate c, char piece);

/* Gegeben eine Ursprungs- und eine Zielkoordinate, überprüft ob es erlaubt wäre, den Turm so zu bewegen. Wenn ja,
 * gibt 0 zurück; wenn nein, gibt -1 zurück. */
int checkMove(coordinate origin, coordinate target);

/* Verschiebt einen Turm von einer angegebenen Koordinate an eine andere angegebene Koordinate. Dieser Zug wird, soweit
 * möglich, ausgeführt, ohne Rücksicht auf Gültigkeit zu nehmen. Es sollten also nur Koordinaten verwendet werden,
 * deren Regelkonformität vorher mit checkMove geprüft wurden. */
void moveTower(coordinate origin, coordinate target);

/* Macht eine moveTower-Operation rückgängig. Die Parameterreihenfolge bleibt gleich. Sollte nicht eigenständig
 * aufgerufen werden, sondern nur in Folge einer gültigen moveTower-Operation. */
void undoMoveTower(coordinate origin, coordinate target);

/* Nimmt die Ursprungs- und Zielkoordinaten einer (gültigen) Turmbewegung entgegen und gibt ein Rating zurück, wie gut
 * dieser Zug ist. Im Moment ist jedes Rating einfach 1. */
float evaluateMove(coordinate origin, coordinate target);

/* Gegeben eine Koordinate, an der ein Turm steht. Testet, an welche Felder der Turm verschoben werden kann.
 * (Im Moment nur als Kommandozeilenausgabe) */
move tryAllMoves(coordinate origin);

/* Gegeben eine Ursprungs- und eine Zielkoordinate, überprüft ob es erlaubt wäre, mit dem Turm so zu schlagen.
 * [Beachte: Target ist nicht das Feld des Turms, der geschlagen wird, sondern das Feld dahinter, auf dem der
 * eigene Turm abgestellt wird.] Wenn ja, gibt 0 zurück; wenn nein, gibt -1 zurück. */
int checkCapture(coordinate origin, coordinate target);

/* Nimmt den Turm, der an der Koordinate origin steht, schlägt den Turm, vor dem Feld target steht,
 * (d.h. entfernt den obersten Spielstein und fügt ihn zum eigenen Turm hinzu) und setzt den Ergebnisturm dann an die
 * Koordinate target. Dieser Zug wird, soweit möglich, ausgeführt, ohne Rücksicht auf Gültigkeit zu nehmen.
 * Es sollten also nur Koordinaten verwendet werden, deren Regelkonformität vorher mit checkCapture geprüft wurden. */
void captureTower(coordinate origin, coordinate target);

/* Macht eine captureTower-Operation rückgängig. Die Parameterreihenfolge bleibt gleich. Es wird angenommen, dass die
 * Funktion nicht eigenständig aufgerufen wird, sondern nur in Folge einer gültigen captureTower-Operation. */
void undoCaptureTower(coordinate origin, coordinate target);

/* Nimmt die Ursprungs- und Zielkoordinaten eines (gültigen!) Schlagzugs entgegen und gibt ein Rating zurück,
 * wie gut dieser Zug ist. Im Moment ist jedes Rating einfach 1. */
float evaluateCapture(coordinate origin, coordinate target);

/* Gegeben eine Koordinate, an der ein Turm steht. Testet alle Schläge, die der Turm an der ersten Koordinate ausführen
 * könnte, und gibt den besten davon zurück. */
move tryAllCaptures(coordinate origin);

move tryCaptureAgain(coordinate nowBlocked, coordinate newOrigin);

/* Nimmt als Parameter einen String (sollte 33 Chars aufnehmen können), ein Spielbrett und eine Koordinate.
 * Repräsentiert alle Spielsteine, aus denen der Turm an der angegebenen Kooordinate besteht, als Folge von
 * Buchstaben (z.B. Bwwbw) und schreibt das Ergebnis in den String.
 */
void towerToString(char *target, coordinate c);

/* Wandelt einen Buchstabe-Zahl-Code in den Datentyp coordinate um.
 * Bei ungültigen Koordinaten (alles außer A-H und 1-8) sind die Koordinaten -1, -1. */
coordinate codeToCoord(char code[2]);

/* Wandelt zwei gegebenene Ganzzahlen (0-7) in den Datentyp coordinate um.
 * Bei ungültigen Werten sind die Koordinaten -1, -1. */
coordinate numsToCoord(int x, int y);

/* Nimmt als Parameter einen String (sollte 3) und eine coordinate. Wandelt die Koordinate um in die
 * Buchstabe-Zahl-Repräsentation (z.B. x = 4, y = 2 --> B3) und schreibt das Ergebnis in den String */
void coordToCode (char* target, coordinate c);

// erzeugt ein neues struct moveInfo und initialisiert es mit Standardwerten
move createMoveStruct(void);

/* Bestimmt den günstigen Spielzug auf Basis des aktuellen Spielbretts. Schreibt diesen in den angegebenen String. */
void think(char *answer);

#endif //SYSPRAK_THINKERFUNCTIONS_H
