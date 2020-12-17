#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "thinkerfunctions.h"
#include "shmfunctions.h"

static int towerAllocCounter = 0;
static unsigned int sizeOfTowerMalloc;
static tower *pointerToStart;
static tower **board;

static bool justConvertedToQueen = false;

/* In der Main-Methode sollte ein Speicher für ein Spielbrett (64 Pointer auf tower) gemalloced werden. Der entstehende
 * Pointer muss einmalig mit dieser Funktion an dieses Modul übergeben werden, damit die statische Variable tower **board
 * gesetzt werden kann, die dann die meisten Funktionen im Modul benutzen. */
void setUpBoard(tower **pBoard) {
    board = pBoard;
}

/* In der Main-Methode sollte Speicher für eine bestimmte Anzahl an Türmen gemalloced werden. Der entstehende Pointer
 * sowie die Anzahl der Türme, die in den Speicher passen, müssen einmalig mit dieser Funktion an dieses Modul
 * übergeben werden, damit statische Variablen gesetzt werden können, die dann andere Funktionen benutzen wollen. */
void setUpTowerAlloc(tower *pStart, unsigned int numTowers) {
    sizeOfTowerMalloc = numTowers * sizeof(tower);
    pointerToStart = pStart;
}

/* Setzt voraus, dass ein Speicherblock für Türme reserviert ist. Gibt einen Pointer auf einen Abschnitt des
 * Speicherblocks zurück, der genau groß genug für einen tower ist. Intern wird ein Zähler
 * versetzt, sodass der nächste Funktionsaufruf einen anderen Abschnitt liefert. */
tower *towerAlloc(void) {
    if (sizeOfTowerMalloc < (towerAllocCounter + 1) * sizeof(tower)) {
        fprintf(stderr, "Fehler! Speicherblock ist bereits voll. Kann keinen Speicher mehr zuteilen.\n");
        printf("towerAllocCounter ist gerade: %d\n", towerAllocCounter);
        return (tower *)NULL;
    } else {
        tower *pTemp = pointerToStart + towerAllocCounter;
        towerAllocCounter += 1;
        return pTemp;
    }
}

/* Setzt den Zähler, den towerAlloc verwendet wieder zurück. Zu verwenden, wenn (in einer neuen Iteration) wieder
 * alle Türme auf das Spielbrett gesetzt werden sollen. */
void resetTallocCounter(void) {
    towerAllocCounter = 0;
}

/* Setzt alle Pointer des Spielbretts auf NULL zurück. */
void resetBoard(void) {
    for (int i = 0; i < 64; i++) {
        board[i] = NULL;
    }
}


// Gibt den zu einer Koordinate auf dem Spielfeld gehörigen Pointer zurück; oder NULL bei ungültiger Koordinate.
tower *getPointerToSquare(coordinate c) {
    if (c.xCoord == -1 || c.yCoord == -1) {
        fprintf(stderr, "Fehler! Ungültige Koordinate.\n");
        return NULL;
    } else {
        return board[8*c.yCoord + c.xCoord];
    }
}

/* Gegeben eine Koordinate, gibt den obersten Spielstein, der auf diesem Feld liegt, als Char zurück;
 * oder 'X' bei ungültiger Koordinate. */
char getTopPiece(coordinate c) {
    if (getPointerToSquare(c) == NULL) {
        return 'X';
    } else {
        return getPointerToSquare(c)->piece;
    }
}

// Druckt zu jedem Feld auf dem Spielbrett den obersten Spielstein formatiert aus.
void printTopPieces(void) {
    printf("  A B C D E F G H\n");
    printf(" +-----------------+\n");
    for (int i = 7; i >= 0; i--) {
        printf("%d| ", i+1);
        for (int j = 0; j < 8; j++) {
            if (getPointerToSquare(numsToCoord(j, i)) == (tower *)NULL) {
                printf("- ");
            } else {
                printf("%c ", getTopPiece(numsToCoord(j, i)));
            }
        }
        printf("|%d\n", i+1);
    }
    printf(" +-----------------+\n");
    printf("   A B C D E F G H\n");
}

/* Wendet printTopPieces an und druckt zusätzlich eine Liste mit allen weißen und schwarzen Türmen.*/
void printFull(void) {
    printTopPieces();

    printf("\nWhite Towers\n");
    printf("============\n");
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            char topPiece = getTopPiece(numsToCoord(i, j));
            if (topPiece == 'w' || topPiece == 'W') {
                char *towerBuffer = malloc(33 * sizeof(char));
                char *coordinateBuffer = malloc(3 * sizeof(char));
                towerToString(towerBuffer, numsToCoord(i, j));
                coordToCode(coordinateBuffer, numsToCoord(i, j));
                printf("%s: %s\n", coordinateBuffer, towerBuffer);
                free(towerBuffer);
                free(coordinateBuffer);
            }
        }
    }

    printf("\nBlack Towers\n");
    printf("============\n");
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            char topPiece = getTopPiece(numsToCoord(j, i));
            if (topPiece == 'b' || topPiece == 'B') {
                char *towerBuffer = malloc(33 * sizeof(char));
                char *coordinateBuffer = malloc(3 * sizeof(char));
                towerToString(towerBuffer, numsToCoord(j, i));
                coordToCode(coordinateBuffer, numsToCoord(j, i));
                printf("%s: %s\n", coordinateBuffer, towerBuffer);
                free(towerBuffer);
                free(coordinateBuffer);
            }
        }
    }
}

/* Erzeugt einen neuen tower/Spielstein und setzt ihn auf das Spielbrett. Verlangt dazu als Parameter eine
 * Zielkoordinate und die Art des Spielsteins (als Char). */
int addToSquare(coordinate c, char piece) {
    if (c.xCoord == -1 || c.yCoord == -1) {
        fprintf(stderr, "Fehler! Ungültige Koordinaten.\n");
        return -1;
    } else {
        tower *pNewTower;
        pNewTower = towerAlloc();

        pNewTower->piece = piece;
        pNewTower->next = getPointerToSquare(c);
        board[8*c.yCoord + c.xCoord] = pNewTower;
        return 0;
    }
}

/* Verschiebt einen Turm von einer angegebenen Koordinate an eine andere angegebene Koordinate. Druckt eine
 * Fehlermeldung, wenn das Ursprungsfeld leer oder das Zielfeld schon besetzt ist. Alle anderen Züge werden akzeptiert
 * (z.B. auch A1 -> H7), d.h. es muss beachtet werden, dass nur regelkonforme Züge an die Funktion übergeben werden.
 * Gibt 0 bei Erfolg und -1 bei Fehler zurück. */
int moveTower(coordinate origin, coordinate target) {
    if (origin.xCoord == target.xCoord && origin.yCoord == target.yCoord) {
        fprintf(stderr, "Fehler! Ursprungs- und Zielfeld sind gleich.\n");
        return -1;
    } else if (abs(origin.xCoord-target.xCoord) != abs(origin.yCoord-target.yCoord)) {
        fprintf(stderr, "Fehler! Ursprungs- und Zielfeld liegen nicht auf einer Diagonale.\n");
        return -1;
    } else if (getPointerToSquare(origin) == NULL) {
        char code[3];
        coordToCode(code, origin);
        fprintf(stderr, "Fehler! Auf dem Ursprungsfeld %s liegt liegt gar kein Stein.\n", code);
        return -1;
    } else if (getPointerToSquare(target) != NULL) {
        char code[3];
        coordToCode(code, target);
        fprintf(stderr, "Fehler! Auf dem Zielfeld %s liegt schon ein Stein.\n", code);
        return -1;
    } else if (getTopPiece(origin) == 'w' && target.yCoord-origin.yCoord != 1) {
        fprintf(stderr, "Fehler! Ein normaler weißer Turm darf nur genau ein Feld diagonal nach vorne ziehen.\n");
        return -1;
    } else if (getTopPiece(origin) == 'b' && target.yCoord-origin.yCoord != -1) {
        fprintf(stderr, "Fehler! Ein normaler schwarzer Turm darf nur genau ein Feld diagonal nach vorne ziehen.\n");
        return -1;
    } else {
        // Spielsteine umsetzen
        board[8*target.yCoord + target.xCoord] = getPointerToSquare(origin);
        board[8*origin.yCoord + origin.xCoord] = NULL;

        // evtl. Verwandlung in Dame
        if (getTopPiece(target) == 'w' & target.yCoord == 7) {
            getPointerToSquare(target)->piece = 'W';
            justConvertedToQueen = true;
        } else if (getTopPiece(target) == 'b' & target.yCoord == 0) {
            getPointerToSquare(target)->piece = 'B';
            justConvertedToQueen = true;
        }

        return 0;
    }
}

/* Macht eine moveTower-Operation rückgängig. Die Parameterreihenfolge bleibt gleich.
 * Gibt keinen Kontrollwert zurück, weil angenommen wird, dass die Funktion nicht eigenständig aufgerufen wird,
 * sondern nur in Folge einer gültigen moveTower-Operation (-> dort Rückgabewert checken) */
void undoMoveTower(coordinate origin, coordinate target) {
    // evtl. Damenumwandlung rückgängig machen
    if (getTopPiece(target) == 'W' && justConvertedToQueen) {
        getPointerToSquare(target)->piece = 'w';
        justConvertedToQueen = false;
    } else if (getTopPiece(target) == 'B' && justConvertedToQueen) {
        getPointerToSquare(target)->piece = 'b';
        justConvertedToQueen = false;
    }

    moveTower(target, origin);
}


/* Nimmt den Turm, der an der Koordinate origin steht, schlägt den Turm, der an victim steht,
 * (d.h. entfernt den obersten Spielstein und fügt ihn zum eigenen Turm hinzu) und setzt den Ergebnisturm dann an die
 * Koordinate target.
 *
 * Gibt eine Fehlermeldung aus, wenn origin oder victim leer sind oder target schon besetzt ist, sowie wenn die
 * Diagonale zwischen origin und victim nicht frei ist; oder wenn victim nicht zu target passt.
 * Ansonsten muss aber selbst sichergestellt werden, dass die übergebenen Züge regelkonform sind.
 *
 * Gibt 0 bei Erfolg und -1 bei Fehler zurück. */
int captureTower(coordinate origin, coordinate target) {
    if (origin.xCoord == target.xCoord && origin.yCoord == target.yCoord) {
        fprintf(stderr, "Fehler! Ursprungs- und Zielfeld sind gleich.\n");
        return -1;
    } else if (abs(origin.xCoord-target.xCoord) != abs(origin.yCoord-target.yCoord)) {
        fprintf(stderr, "Fehler! Ursprungs- und Zielfeld liegen nicht auf einer Diagonale.\n");
        return -1;
    } else if ((getTopPiece(origin) == 'b' || getTopPiece(origin) == 'w') && abs(origin.xCoord-target.xCoord) != 2) {
        fprintf(stderr, "Fehler! Um mit einem normalen Turm zu schlagen, müssen Ursprungs- und Zielfeld diagonal genau zwei Felder entfernt sein.\n");
        return -1;
    } else if ((getTopPiece(origin) == 'B' || getTopPiece(origin) == 'B') && abs(origin.xCoord-target.xCoord) < 2) {
        fprintf(stderr, "Fehler! Um mit einer Dame zu schlagen, müssen Ursprungs- und Zielfeld diagonal mindestens zwei Felder entfernt sein.\n");
        return -1;
    }

    // berechne die Koordinate für das Feld victim
    int xDirection = getSign(target.xCoord-origin.xCoord);
    int yDirection = getSign(target.yCoord-origin.yCoord);
    coordinate victim;
    victim.xCoord = target.xCoord - xDirection;
    victim.yCoord = target.yCoord - yDirection;

    if ((getTopPiece(origin) == 'b' || getTopPiece(origin) == 'B') && (getTopPiece(victim) == 'b' || getTopPiece(victim) == 'B')) {
        fprintf(stderr, "Fehler! Ein schwarzer Turm kann keinen anderen schwarzen Turm schlagen.\n");
        return -1;
    } else if ((getTopPiece(origin) == 'w' || getTopPiece(origin) == 'W') && (getTopPiece(victim) == 'w' || getTopPiece(victim) == 'W')) {
        fprintf(stderr, "Fehler! Ein weißer Turm kann keinen anderen weißen Turm schlagen.\n");
        return -1;
    }

    // überprüft, ob auf den Feldern zwischen Origin und Victim etwas steht
    int i = origin.xCoord + xDirection;
    int j = origin.xCoord + yDirection;
    while (i != victim.xCoord) {
        if (getPointerToSquare(numsToCoord(i, j)) != NULL) {
            char code[3];
            coordToCode(code, numsToCoord(i,j));
            fprintf(stderr, "Fehler! Auf dem Feld %s zwischen Origin und Victim steht ein Turm. Schlagen ist deshalb nicht möglich.\n", code);
            return -1;
        }
        i += xDirection;
        j += yDirection;
    }

    if (getPointerToSquare(origin) == NULL) {
        char code[3];
        coordToCode(code, origin);
        fprintf(stderr, "Fehler! Auf dem Ursprungsfeld %s liegt gar kein Stein.\n", code);
        return -1;
    } else if (getPointerToSquare(victim) == NULL) {
        char code[3];
        coordToCode(code, victim);
        fprintf(stderr, "Fehler! Auf dem Feld %s ist gar kein Stein, der geschlagen werden könnte.\n", code);
        return -1;
    } else if (getPointerToSquare(target) != NULL) {
        char code[3];
        coordToCode(code, target);
        fprintf(stderr, "Fehler! Auf dem Zielfeld %s steht schon ein Turm.\n", code);
        return -1;
    }


    // ab hier tatsächliche Versetzung der Spielsteine
    tower *pNewVictimTower = getPointerToSquare(victim)->next;
    tower *pTowerForTarget = getPointerToSquare(origin);

    tower *pCurrent = pTowerForTarget;
    while (pCurrent->next != NULL) {
        pCurrent = pCurrent->next;
    }
    pCurrent->next = getPointerToSquare(victim);
    pCurrent->next->next = NULL;

    board[8*victim.yCoord + victim.xCoord] = pNewVictimTower;
    board[8*target.yCoord + target.xCoord] = pTowerForTarget;
    board[8*origin.yCoord + origin.xCoord] = NULL;

    // evtl. Verwandlung in Dame
    if (getTopPiece(target) == 'w' & target.yCoord == 7) {
        getPointerToSquare(target)->piece = 'W';
        justConvertedToQueen = true;
    } else if (getTopPiece(target) == 'b' & target.yCoord == 0) {
        getPointerToSquare(target)->piece = 'B';
        justConvertedToQueen = true;
    }

    return 0;
}

/* Macht eine captureTower-Operation rückgängig. Die Parameterreihenfolge bleibt gleich.
 * Gibt keinen Kontrollwert zurück, weil angenommen wird, dass die Funktion nicht eigenständig aufgerufen wird,
 * sondern nur in Folge einer gültigen captureTower-Operation (-> dort Rückgabewert checken) */
void undoCaptureTower(coordinate origin, coordinate target) {

    // berechne die Koordinate für das Feld victim
    int xDirection = getSign(target.xCoord-origin.xCoord);
    int yDirection = getSign(target.yCoord-origin.yCoord);
    coordinate victim;
    victim.xCoord = target.xCoord - xDirection;
    victim.yCoord = target.yCoord - yDirection;

    // evtl. Damenumwandlung rückgängig machen
    if (getTopPiece(target) == 'W' && justConvertedToQueen) {
        getPointerToSquare(target)->piece = 'w';
        justConvertedToQueen = false;
    } else if (getTopPiece(target) == 'B' && justConvertedToQueen) {
        getPointerToSquare(target)->piece = 'b';
        justConvertedToQueen = false;
    }

    tower *pCurrent = getPointerToSquare(target);
    tower *pBottomPieceOfTarget;
    while (pCurrent->next->next != NULL) {
        pCurrent = pCurrent->next;
    }
    pBottomPieceOfTarget = pCurrent->next;
    pCurrent->next = NULL;

    pBottomPieceOfTarget->next = getPointerToSquare(victim);
    board[8*victim.yCoord+victim.xCoord] = pBottomPieceOfTarget;

    board[8*origin.yCoord+origin.xCoord] = getPointerToSquare(target);
    board[8*target.yCoord+target.xCoord] = NULL;
}

/* Nimmt als Parameter einen String (sollte 33 Chars aufnehmen können), ein Spielbrett und eine Koordinate.
 * Repräsentiert alle Spielsteine, aus denen der Turm an der angegebenen Kooordinate besteht, als Folge von
 * Buchstaben (z.B. Bwwbw) und schreibt das Ergebnis in den String.
 */
void towerToString(char *target, coordinate c) {
    char ret[32+1];
    tower *pCurrent = getPointerToSquare(c);

    int i = 0;
    while (i < 32) {
        if (pCurrent == NULL) break;
        ret[i] = pCurrent->piece;
        pCurrent = pCurrent->next;
        i++;
    }
    ret[i+1] = '\0';
    strncpy(target, ret, 33);
}

/* Wandelt einen Buchstabe-Zahl-Code in den Datentyp coordinate um.
 * Bei ungültigen Koordinaten (alles außer A-H und 1-8) sind die Koordinaten -1, -1. */
coordinate codeToCoord(char code[2]) {
    coordinate ret;
    switch(code[0]) {
        case 'A':
        case 'a':
            ret.xCoord = 0;
            break;
        case 'B':
        case 'b':
            ret.xCoord = 1;
            break;
        case 'C':
        case 'c':
            ret.xCoord = 2;
            break;
        case 'D':
        case 'd':
            ret.xCoord = 3;
            break;
        case 'E':
        case 'e':
            ret.xCoord = 4;
            break;
        case 'F':
        case 'f':
            ret.xCoord = 5;
            break;
        case 'G':
        case 'g':
            ret.xCoord = 6;
            break;
        case 'H':
        case 'h':
            ret.xCoord = 7;
            break;
        default:
            ret.xCoord = -1;
            break;
    }
    switch(code[1]) {
        case '1':
            ret.yCoord = 0;
            break;
        case '2':
            ret.yCoord = 1;
            break;
        case '3':
            ret.yCoord = 2;
            break;
        case '4':
            ret.yCoord = 3;
            break;
        case '5':
            ret.yCoord = 4;
            break;
        case '6':
            ret.yCoord = 5;
            break;
        case '7':
            ret.yCoord = 6;
            break;
        case '8':
            ret.yCoord = 7;
            break;
        default:
            ret.yCoord = -1;
            break;
    }
    return ret;
}

/* Wandelt zwei gegebenene Ganzzahlen (0-7) in den Datentyp coordinate um.
 * Bei ungültigen Werten sind die Koordinaten -1, -1. */
coordinate numsToCoord(int x, int y) {
    coordinate ret;
    if (x >= 0 && x <= 7) ret.xCoord = x;
    else ret.xCoord = -1;
    if (y >= 0 && y <= 7) ret.yCoord = y;
    else ret.yCoord = -1;
    return ret;
}

/* Nimmt als Parameter einen String (sollte 3) und eine coordinate. Wandelt die Koordinate um in die
 * Buchstabe-Zahl-Repräsentation (z.B. x = 4, y = 2 --> B3) und schreibt das Ergebnis in den String */
void coordToCode(char *target, coordinate c) {
    char ret[3];

    switch(c.xCoord) {
        case 0:
            ret[0] = 'A';
            break;
        case 1:
            ret[0] = 'B';
            break;
        case 2:
            ret[0] = 'C';
            break;
        case 3:
            ret[0] = 'D';
            break;
        case 4:
            ret[0] = 'E';
            break;
        case 5:
            ret[0] = 'F';
            break;
        case 6:
            ret[0] = 'G';
            break;
        case 7:
            ret[0] = 'H';
            break;
        default:
            ret[0] = 'X';
            break;
    }
    switch(c.yCoord) {
        case 0:
            ret[1] = '1';
            break;
        case 1:
            ret[1] = '2';
            break;
        case 2:
            ret[1] = '3';
            break;
        case 3:
            ret[1] = '4';
            break;
        case 4:
            ret[1] = '5';
            break;
        case 5:
            ret[1] = '6';
            break;
        case 6:
            ret[1] = '7';
            break;
        case 7:
            ret[1] = '8';
            break;
        default:
            ret[1] = 'X';
            break;
    }

    ret[2] = '\0';

    strncpy(target, ret, 3);
}

int minInt(int a, int b) {
    if (b < a) return b;
    else return a;
}

int maxInt(int a, int b) {
    if (b > a) return b;
    else return a;
}

int getSign(int a) {
    if (a == 0) return 0;
    else if (a > 0) return 1;
    else return -1;
}

int abs(int a) {
    if (a >= 0) return a;
    else return -a;
}