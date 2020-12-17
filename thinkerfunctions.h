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

/* In der Main-Methode sollte ein Spielbrett (tower*) gemalloced werden. Der entstehende Pointer muss einmalig mit
 * dieser Funktion an dieses Modul übergeben werden, damit die statische Variable tower **board gesetzt werden kann,
 * die dann die meisten Funktionen im Modul benutzen. */
void setUpBoard(tower **pBoard);

/* In der Main-Methode sollte Speicher für eine bestimmte Anzahl an Türmen gemalloced werden. Der entstehende Pointer
 * sowie die Anzahl der Türme, die in den Speicher passen, müssen einmalig mit dieser Funktion an dieses Modul
 * übergeben werden, damit statische Variablen gesetzt werden können, die dann andere Funktionen benutzen wollen. */
void setUpTowerAlloc(tower *pStart, unsigned int numTowers);

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

/* Verschiebt einen Turm von einer angegebenen Koordinate an eine andere angegebene Koordinate. Druckt eine
 * Fehlermeldung, wenn das Ursprungsfeld leer oder das Zielfeld schon besetzt ist. Alle anderen Züge werden akzeptiert
 * (z.B. auch A1 -> H7), d.h. es muss beachtet werden, dass nur regelkonforme Züge an die Funktion übergeben werden.
 * Gibt 0 bei Erfolg und -1 bei Fehler zurück. */
int moveTower(coordinate origin, coordinate target);

/* Macht eine moveTower-Operation rückgängig. Die Parameterreihenfolge bleibt gleich.
 * Gibt 0 bei Erfolg und -1 bei Fehler zurück. */
int undoMoveTower(coordinate origin, coordinate target);

/* Nimmt den Turm, der an der Koordinate origin steht, schlägt den Turm, der an victim steht,
 * (d.h. entfernt den obersten Spielstein und fügt ihn zum eigenen Turm hinzu) und setzt den Ergebnisturm dann an die
 * Koordinate target.
 *
 * Gibt eine Fehlermeldung aus, wenn origin oder victim leer sind oder target schon besetzt ist, sowie wenn die
 * Diagonale zwischen origin und victim nicht frei ist; oder wenn victim nicht zu target passt.
 * Ansonsten muss aber selbst sichergestellt werden, dass die übergebenen Züge regelkonform sind.
 *
 * Gibt 0 bei Erfolg und -1 bei Fehler zurück. */
int beatTower(coordinate origin, coordinate victim, coordinate target);

/* Macht eine beatTower-Operation rückgängig. Die Parameterreihenfolge bleibt gleich.
 * Gibt 0 bei Erfolg und -1 bei Fehler zurück. */
int undoBeatTower(coordinate origin, coordinate victim, coordinate target);

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

int minInt(int a, int b);

int maxInt(int a, int b);

int getSign(int a);

#endif //SYSPRAK_THINKERFUNCTIONS_H
