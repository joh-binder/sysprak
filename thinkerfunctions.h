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

/* Setzt voraus, dass ein Speicherblock reserviert ist. Nimmt als Parameter einen Pointer auf den Anfang dieses
 * Speicherblocks und die Blockgröße; außerdem eine Wunschgröße. Gibt dann einen Pointer auf einen Abschnitt
 * des Speicherblocks in der Wunschgröße zurück, insofern genug Platz vorhanden ist. Intern wird ein Zähler
 * versetzt, sodass der nächste Funktionsaufruf einen anderen Abschnitt liefert.
 *
 * [Beachte: Pro Programmaufruf kann diese Methode nur für die Verwaltung eines (1) Speicherblocks benutzt werden.]
 */
tower *towerAlloc(tower *pointerToStart, unsigned long maxSizeOfBlock);

void resetTallocCounter(void);

/* Wandelt einen Buchstabe-Zahl-Code in den Datentyp coordinate um.
 * Bei ungültigen Koordinaten (alles außer A-H und 1-8) sind die Koordinaten -1, -1. */
coordinate codeToCoord(char code[2]);

/* Wandelt zwei gegebenene Ganzzahlen (0-7) in den Datentyp coordinate um.
 * Bei ungültigen Werten sind die Koordinaten -1, -1. */
coordinate numsToCoord(int x, int y);

/* Gegeben ein Array von Pointern auf tower (=ein Spielbrett) und eine Koordinate, gibt den zur Koordinate
 * gehörigen Pointer zurück; oder NULL bei ungültiger Koordinate. */
tower *getPointerToSquare(tower **board, coordinate c);

/* Gegeben ein Array von Pointern auf tower (=ein Spielbrett) und eine Koordinate, gibt den obersten Spielstein,
 * der auf diesem Feld liegt, als Char zurück; oder 'X' bei ungültiger Koordinate. */
char getTopPiece(tower **board, coordinate c);

// Druckt zu einem gegebenen Spielfeld zu jedem Feld den obersten Spielstein formatiert aus.
void printTopPieces(tower *board[64]);

/* Wendet printTopPieces an und druckt zusätzlich eine Liste mit allen weißen und schwarzen Türmen.*/
void printFull(tower **pBoard);

/* Erzeugt einen neuen tower/Spielstein und setzt ihn auf das Spielbrett. Verlangt dazu als Parameter das
 * Spielbrett, eine Zielkoordinate, die Art des Spielsteins (als Char) und einen Pointer auf den
 * Anfangsbereich des (Gesamt-)Speicherblocks, in dem der neue tower gespeichert wird. */
void addToSquare(tower **board, coordinate c, char piece, tower *pointerToAlloc, int maxTowersNeeded);

/* Verschiebt auf einem gegebenen Array von Pointern auf tower (=einem Spielbrett) den Turm, der an einer
 * gegebenen Koordinate steht, an eine andere Koordinate. Druckt eine Fehlermeldung, wenn das Ursprungsfeld
 * leer oder das Zielfeld schon besetzt ist. Alle anderen Züge werden akzeptiert (z.B. auch A1 -> H7),
 * d.h. es muss beachtet werden, dass nur regelkonforme Züge an die Funktion übergeben werden.
 */
void moveTower(tower **board, coordinate origin, coordinate target);

/* Auf einem gegebenen Array von Pointern auf tower (=einem Spielbrett), nimmt den Turm, der an der
 * Koordinate origin steht, schlägt den Turm, der an victim steht, (d.h. entfernt den obersten Spielstein und
 * fügt ihn zum eigenen Turm hinzu) und setzt den Ergebnisturm dann an die Koordinate target.
 *
 * Gibt eine Fehlermeldung aus, wenn origin oder victim leer sind oder target schon besetzt ist,
 * ansonsten wird jeder Zug ausgeführt. Es muss also selbst sichergestellt werden, dass die übergebenen Züge
 * regelkonform sind. */
void beatTower(tower **board, coordinate origin, coordinate victim, coordinate target);

/* Nimmt als Parameter einen String (sollte 33 Chars aufnehmen können), ein Spielbrett und eine Koordinate.
 * Repräsentiert alle Spielsteine, aus denen der Turm an der angegebenen Kooordinate besteht, als Folge von
 * Buchstaben (z.B. Bwwbw) und schreibt das Ergebnis in den String.
 */
void towerToString(char *target, tower **board, coordinate c);

/* Nimmt als Parameter einen String (sollte 3) und eine coordinate. Wandelt die Koordinate um in die
 * Buchstabe-Zahl-Repräsentation (z.B. x = 4, y = 2 --> B3) und schreibt das Ergebnis in den String */
void coordToCode (char* target, coordinate c);

#endif //SYSPRAK_THINKERFUNCTIONS_H
