#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <float.h>

#include "util.h"
#include "thinkerfunctions.h"

#define NUMBER_OF_PIECES_IN_BASHNI 32
#define NUM_ROWS 8 // (auch Zahl der Spalten)

#define QUEEN_TO_NORMAL_RATIO 3.0
#define NORMAL_PIECE_WEIGHT 1.0
#define POSSIBLE_MOVE_BASE_RATING 1.0
#define ADVANCE_NORMAL_PIECE_BONUS 0.5
#define CONVERT_TO_QUEEN_BONUS 5.0
#define OWN_ENDANGERMENT_FACTOR 1.0
#define THREATEN_OPPONENT_FACTOR 0.25
#define CAPTURE_QUEEN_BONUS 1.5
#define ANOTHER_CAPTURE_POSSIBLE_BONUS 1.5
#define WEIGHT_RATING_FACTOR 0.5

/* TODO Kommentieren */

static int towerAllocCounter = 0;
static unsigned int sizeOfTowerMalloc;
static tower *pointerToStart;
static tower **board;

static char ownNormalTower, ownQueenTower, opponentNormalTower, opponentQueenTower;
static bool justConvertedToQueen = false;

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
    if (x >= 0 && x <= NUM_ROWS-1) ret.xCoord = x;
    else ret.xCoord = -1;
    if (y >= 0 && y <= NUM_ROWS-1) ret.yCoord = y;
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

// erzeugt ein neues struct gameInfo und initialisiert es mit Standardwerten
move createMoveStruct(void) {
    move ret;
    ret.origin = numsToCoord(-1, -1);
    ret.target = numsToCoord(-1, -1);
    ret.rating = -FLT_MAX;
    return ret;
}

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

/* Hiermit muss die Spielernummer an thinkerfunctions übergeben werden, damit hier bekannt ist, was die eigene Farbe
 * und was die des Gegners ist. */
int setUpWhoIsWho(int playerno) {
    if (playerno == 0) {
        ownNormalTower = 'w';
        ownQueenTower = 'W';
        opponentNormalTower = 'b';
        opponentQueenTower = 'B';
        return 0;
    } else if (playerno == 1) {
        ownNormalTower = 'b';
        ownQueenTower = 'B';
        opponentNormalTower = 'w';
        opponentQueenTower = 'W';
        return 0;
    } else {
        fprintf(stderr, "Fehler! Eigene Spielernummer ist weder 0 noch 1.\n");
        return -1;
    }
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

/* Bündelt die Funktionen resetTallocCounter und resetBoard, da diese sowieso miteinander verwendet werden sollen. */
void prepareNewRound(void) {
    resetTallocCounter();
    resetBoard();
}

// Gibt den zu einer Koordinate auf dem Spielfeld gehörigen Pointer zurück; oder NULL bei ungültiger Koordinate.
tower *getPointerToSquare(coordinate c) {
    if (c.xCoord == -1 || c.yCoord == -1) {
        fprintf(stderr, "Fehler! Ungültige Koordinate.\n");
        return NULL;
    } else {
        return board[NUM_ROWS * c.yCoord + c.xCoord];
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

/* Nimmt als Parameter einen String (sollte 33 Chars aufnehmen können), ein Spielbrett und eine Koordinate.
 * Repräsentiert alle Spielsteine, aus denen der Turm an der angegebenen Kooordinate besteht, als Folge von
 * Buchstaben (z.B. Bwwbw) und schreibt das Ergebnis in den String.
 */
void towerToString(char *target, coordinate c) {
    char ret[NUMBER_OF_PIECES_IN_BASHNI+1];
    memset(ret, 0, NUMBER_OF_PIECES_IN_BASHNI+1);
    tower *pCurrent = getPointerToSquare(c);

    int i = 0;
    while (i < NUMBER_OF_PIECES_IN_BASHNI) {
        if (pCurrent == NULL) break;
        ret[i] = pCurrent->piece;
        pCurrent = pCurrent->next;
        i++;
    }
    ret[i+1] = '\0';
    strncpy(target, ret, NUMBER_OF_PIECES_IN_BASHNI+1);
}


// Druckt zu jedem Feld auf dem Spielbrett den obersten Spielstein formatiert aus.
void printTopPieces(void) {
    printf("   A B C D E F G H\n");
    printf(" +-----------------+\n");
    for (int i = NUM_ROWS-1; i >= 0; i--) {
        printf("%d| ", i+1);
        for (int j = 0; j < NUM_ROWS; j++) {
            if (getPointerToSquare(numsToCoord(j, i)) == (tower *)NULL) {
                if (j % 2 == i % 2) printf("_ ");
                else printf(". ");
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
    for (int i = 0; i < NUM_ROWS; i++) {
        for (int j = 0; j < NUM_ROWS; j++) {
            char topPiece = getTopPiece(numsToCoord(i, j));
            if (topPiece == 'w' || topPiece == 'W') {
                char towerBuffer[NUMBER_OF_PIECES_IN_BASHNI+1];
                char coordinateBuffer[3];
                towerToString(towerBuffer, numsToCoord(i, j));
                coordToCode(coordinateBuffer, numsToCoord(i, j));
                printf("%s: %s\n", coordinateBuffer, towerBuffer);
            }
        }
    }

    printf("\nBlack Towers\n");
    printf("============\n");
    for (int i = 0; i < NUM_ROWS; i++) {
        for (int j = 0; j < NUM_ROWS; j++) {
            char topPiece = getTopPiece(numsToCoord(j, i));
            if (topPiece == 'b' || topPiece == 'B') {
                char towerBuffer[NUMBER_OF_PIECES_IN_BASHNI+1];
                char coordinateBuffer[3];
                towerToString(towerBuffer, numsToCoord(j, i));
                coordToCode(coordinateBuffer, numsToCoord(j, i));
                printf("%s: %s\n", coordinateBuffer, towerBuffer);
            }
        }
    }
    printf("\n");
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
        board[NUM_ROWS * c.yCoord + c.xCoord] = pNewTower;
        return 0;
    }
}

/* Gegeben eine Ursprungs- und eine Zielkoordinate, überprüft ob es erlaubt wäre, den Turm so zu bewegen. Wenn ja,
 * gibt 0 zurück; wenn nein, gibt -1 zurück. */
int checkMove(coordinate origin, coordinate target) {
    if (origin.xCoord == -1 || origin.yCoord == -1) {
        return -1; // ungültige Ursprungskoordinate
    } else if (target.xCoord == -1 || target.yCoord == -1) {
        return -1; // ungültige Zielkoordinate
    } else if (origin.xCoord == target.xCoord && origin.yCoord == target.yCoord) {
        return -1; // Ziel- und Ursprungskoordiante gleich
    } else if (abs(origin.xCoord-target.xCoord) != abs(origin.yCoord-target.yCoord)) {
        return -1; // Ziel- und Ursprungskoordinate liegen nicht auf einer Diagonale
    } else if (getPointerToSquare(origin) == NULL) {
        return -1; // Ursprungsfeld ist leer
    } else if (getPointerToSquare(target) != NULL) {
        return -1; // Zielfeld ist NICHT leer
    } else {
        char originTopPiece = getTopPiece(origin);
        int originTargetSignedYDistance = target.yCoord-origin.yCoord;
        if (originTopPiece == 'w' && originTargetSignedYDistance != 1) {
            return -1; // ein normaler weißer Turm darf nur genau ein Feld diagonal vorwärts (in positive Y-Richtung) ziehen
        } else if (originTopPiece == 'b' && originTargetSignedYDistance != -1) {
            return -1; // ein normaler schwarzer Turm darf nur genau ein Feld diagonal vorwärts (in negative Y-Richtung) ziehen
        }
    }

    // überprüft, ob auf den Feldern zwischen Origin und Target etwas steht
    int xDirection = getSign(target.xCoord-origin.xCoord);
    int yDirection = getSign(target.yCoord-origin.yCoord);
    int i = origin.xCoord + xDirection;
    int j = origin.yCoord + yDirection;
    while (i != target.xCoord) {
        if (getPointerToSquare(numsToCoord(i, j)) != NULL) {
            return -1; // zwischen Ursprungs- und Zielfeld steht ein anderer Turm
        }
        i += xDirection;
        j += yDirection;
    }

    return 0;
}

/* Verschiebt einen Turm von einer angegebenen Koordinate an eine andere angegebene Koordinate. Dieser Zug wird, soweit
 * möglich, ausgeführt, ohne Rücksicht auf Gültigkeit zu nehmen. Es sollten also nur Koordinaten verwendet werden,
 * deren Regelkonformität vorher mit checkMove geprüft wurden. */
void moveTower(coordinate origin, coordinate target) {
    // Spielsteine umsetzen
    board[NUM_ROWS * target.yCoord + target.xCoord] = getPointerToSquare(origin);
    board[NUM_ROWS * origin.yCoord + origin.xCoord] = NULL;

    // evtl. Verwandlung in Dame
    if (getTopPiece(target) == 'w' && target.yCoord == NUM_ROWS-1) {
        getPointerToSquare(target)->piece = 'W';
        justConvertedToQueen = true;
    } else if (getTopPiece(target) == 'b' && target.yCoord == 0) {
        getPointerToSquare(target)->piece = 'B';
        justConvertedToQueen = true;
    }
}

/* Macht eine moveTower-Operation rückgängig. Die Parameterreihenfolge bleibt gleich. Sollte nicht eigenständig
 * aufgerufen werden, sondern nur in Folge einer gültigen moveTower-Operation. */
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

/* Gegeben die Koordinaten für origin und target, berechnet die Koordinaten für Victim (d.h. ein Feld vor Target aus der Richtung, in der Origin ist).
 * Liefert vermutlich kein richtiges Ergebnis, wenn origin und target nicht auf einer Diagonale stehen -> vorher prüfen. */
coordinate computeVictim(coordinate origin, coordinate target) {
    int xDirection = getSign(target.xCoord-origin.xCoord);
    int yDirection = getSign(target.yCoord-origin.yCoord);
    coordinate victim;
    victim.xCoord = target.xCoord - xDirection;
    victim.yCoord = target.yCoord - yDirection;
    return victim;
}

/* Gegeben eine Ursprungs- und eine Zielkoordinate, überprüft ob es erlaubt wäre, mit dem Turm so zu schlagen.
 * [Beachte: Target ist nicht das Feld des Turms, der geschlagen wird, sondern das Feld dahinter, auf dem der
 * eigene Turm abgestellt wird.] Wenn ja, gibt 0 zurück; wenn nein, gibt -1 zurück. */
int checkCapture(coordinate origin, coordinate target) {
    if (origin.xCoord == -1 || origin.yCoord == -1) {
        return -1; // ungültige Ursprungskoordinate
    } else if (target.xCoord == -1 || target.yCoord == -1) {
        return -1; // ungültige Zielkoordinate
    } else if (origin.xCoord == target.xCoord && origin.yCoord == target.yCoord) {
        return -1; // Ursprungs- und Zielfeld sind gleich
    } else if (abs(origin.xCoord-target.xCoord) != abs(origin.yCoord-target.yCoord)) {
        return -1; // Ursprungs- und Zielfeld liegen nicht auf einer Diagonale
    }

    char originTopPiece = getTopPiece(origin);
    int originTargetDistance = abs(origin.xCoord-target.xCoord);
    if ((originTopPiece == 'b' || originTopPiece == 'w') && originTargetDistance != 2) {
        return -1; // Um mit einem normalen Turm zu schlagen, müssen Ursprungs- und Zielfeld diagonal genau zwei Felder entfernt sein.
    } else if ((originTopPiece == 'B' || originTopPiece == 'W') && originTargetDistance < 2) {
        return -1; // Um mit einer Dame zu schlagen, müssen Ursprungs- und Zielfeld diagonal mindestens zwei Felder entfernt sein.
    }

    coordinate victim = computeVictim(origin, target);
    char victimTopPiece = getTopPiece(victim);
    if ((originTopPiece == 'b' || originTopPiece == 'B') && (victimTopPiece == 'b' || victimTopPiece == 'B')) {
        return -1; // Ein schwarzer Turm kann keinen anderen schwarzen Turm schlagen.
    } else if ((originTopPiece == 'w' || originTopPiece == 'W') && (victimTopPiece == 'w' || victimTopPiece == 'W')) {
        return -1; // Ein weißer Turm kann keinen anderen weißen Turm schlagen.
    }

    // überprüft, ob auf den Feldern zwischen Origin und Victim etwas steht
    int xDirection = getSign(target.xCoord-origin.xCoord);
    int yDirection = getSign(target.yCoord-origin.yCoord);
    int i = origin.xCoord + xDirection;
    int j = origin.yCoord + yDirection;
    while (i != victim.xCoord) {
        if (getPointerToSquare(numsToCoord(i, j)) != NULL) {
            return -1; // Auf einem Feld zwischen Origin und Victim steht schon ein Turm
        }
        i += xDirection;
        j += yDirection;
    }

    if (getPointerToSquare(origin) == NULL) {
        return -1; // auf dem Ursprungsfeld steht gar kein Turm
    } else if (getPointerToSquare(victim) == NULL) {
        return -1; // auf dem Victim-Feld ist gar kein Turm, der geschlagen werden könnte
    } else if (getPointerToSquare(target) != NULL) {
        return -1; // auf dem Zielfeld steht schon ein Turm
    }

    return 0;
}

/* Gegeben eine Koordinate auf dem Spielbrett, überprüft ob Turm, der auf diesem Feld steht, in Gefahr ist, geschlagen zu werden.
 * Funktioniert nur für Türme in der eigenen Farbe. */
bool isInDanger(coordinate square) {
    int xDirection, yDirection;
    for (int round = 0; round < 4; round++) { // 4, weil vorne links, vorne rechts, hinten links, hinten rechts
        switch (round) {
            case 0:
                xDirection = 1;
                yDirection = 1;
                break;
            case 1:
                xDirection = 1;
                yDirection = -1;
                break;
            case 2:
                xDirection = -1;
                yDirection = 1;
                break;
            case 3:
                xDirection = -1;
                yDirection = -1;
                break;
        }

        coordinate behind = numsToCoord(square.xCoord + xDirection, square.yCoord + yDirection); // berechne das Feld hinter square (wo der gegnerische Turm nach dem Schlagen von square abgestellt werden müsste)

        if (behind.xCoord == -1 || behind.yCoord == -1) continue; // wenn hinter square kein gültiges Feld mehr ist -> in dieser Richtung sicher
        if (getPointerToSquare(behind) != NULL) continue; // wenn Feld hinter square besetzt -> auch sicher

        // betrachte jetzt die Gegenrichtung, also ob VOR square überhaupt ein Stein ist, der schlagen könnte
        xDirection *= -1;
        yDirection *= -1;
        coordinate potentialOpponentPiece;

        for (int i = 1; i <= NUM_ROWS-1-1; i++) { // 6, weil Bashni insgesamt 8 Felder pro Diagonale hat; mit 1 Feld für square und 1 für dahinter bleiben noch 6 Felder, an denen ein schlagender Stein stehen könnte
            potentialOpponentPiece = numsToCoord(square.xCoord + i * xDirection, square.yCoord + i * yDirection);

            if (potentialOpponentPiece.xCoord == -1 || potentialOpponentPiece.yCoord == -1) break;
            if (getPointerToSquare(potentialOpponentPiece) == NULL) continue;
            if (getTopPiece(potentialOpponentPiece) == ownNormalTower || getTopPiece(potentialOpponentPiece) == ownQueenTower) break;
            if (checkCapture(potentialOpponentPiece, behind) == 0) return true;
        }

    }
    return false;
}

/* Zählt, wie viele Türme der eigenen Farbe sich nach Stand des momentanen Spielbretts in Gefahr befinden. Eine Dame zählt 3-fach. */
int howManyInDangerOwn(void) {
    int countTotalInDanger = 0;

    for (int i = 0; i < NUM_ROWS; i++) {
        for (int j = 0; j < NUM_ROWS; j++) {
            coordinate thisCoord = numsToCoord(i,j);
            char topPieceHere = getTopPiece(thisCoord);
            if (topPieceHere != ownNormalTower && topPieceHere != ownQueenTower) {
                continue;
            }
            if (isInDanger(thisCoord)) {
                if (topPieceHere == ownNormalTower) countTotalInDanger += 1;
                else countTotalInDanger += QUEEN_TO_NORMAL_RATIO * 1; // wenn topPieceHere eine Dame ist -> zählt mehrfach
            }
        }
    }
    return countTotalInDanger;
}

void swapOwnAndOppPieces(void) {
    char tempNormalTower, tempQueenTower;
    tempNormalTower = ownNormalTower;
    tempQueenTower = ownQueenTower;
    ownNormalTower = opponentNormalTower;
    ownQueenTower = opponentQueenTower;
    opponentNormalTower = tempNormalTower;
    opponentQueenTower = tempQueenTower;
}

int howManyInDangerOpponent(void) {
    swapOwnAndOppPieces();
    int ret = howManyInDangerOwn();
    swapOwnAndOppPieces();
    return ret;
}

/* Nimmt den Turm, der an der Koordinate origin steht, schlägt den Turm, vor dem Feld target steht,
 * (d.h. entfernt den obersten Spielstein und fügt ihn zum eigenen Turm hinzu) und setzt den Ergebnisturm dann an die
 * Koordinate target. Dieser Zug wird, soweit möglich, ausgeführt, ohne Rücksicht auf Gültigkeit zu nehmen.
 * Es sollten also nur Koordinaten verwendet werden, deren Regelkonformität vorher mit checkCapture geprüft wurden. */
void captureTower(coordinate origin, coordinate target) {
    coordinate victim = computeVictim(origin, target);

    // Versetzung der Spielsteine
    tower *pNewVictimTower = getPointerToSquare(victim)->next;
    tower *pTowerForTarget = getPointerToSquare(origin);

    tower *pCurrent = pTowerForTarget;
    while (pCurrent->next != NULL) {
        pCurrent = pCurrent->next;
    }
    pCurrent->next = getPointerToSquare(victim);
    pCurrent->next->next = NULL;

    board[NUM_ROWS * victim.yCoord + victim.xCoord] = pNewVictimTower;
    board[NUM_ROWS * target.yCoord + target.xCoord] = pTowerForTarget;
    board[NUM_ROWS * origin.yCoord + origin.xCoord] = NULL;

    // evtl. Verwandlung in Dame
    if (getTopPiece(target) == 'w' && target.yCoord == NUM_ROWS-1) {
        getPointerToSquare(target)->piece = 'W';
        justConvertedToQueen = true;
    } else if (getTopPiece(target) == 'b' && target.yCoord == 0) {
        getPointerToSquare(target)->piece = 'B';
        justConvertedToQueen = true;
    }

}

/* Macht eine captureTower-Operation rückgängig. Die Parameterreihenfolge bleibt gleich. Es wird angenommen, dass die
 * Funktion nicht eigenständig aufgerufen wird, sondern nur in Folge einer gültigen captureTower-Operation. */
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

    // ab hier eigentliches Umversetzen
    tower *pCurrent = getPointerToSquare(target);
    tower *pBottomPieceOfTarget;
    while (pCurrent->next->next != NULL) {
        pCurrent = pCurrent->next;
    }

    pBottomPieceOfTarget = pCurrent->next;
    pCurrent->next = NULL;

    pBottomPieceOfTarget->next = getPointerToSquare(victim);
    board[NUM_ROWS * victim.yCoord+victim.xCoord] = pBottomPieceOfTarget;

    board[NUM_ROWS * origin.yCoord+origin.xCoord] = getPointerToSquare(target);
    board[NUM_ROWS * target.yCoord+target.xCoord] = NULL;
}

float computeTowerWeight(coordinate square) {
    float weight = 0;
    tower *pCurrent = getPointerToSquare(square);

    while (pCurrent != NULL) {
        if (pCurrent->piece == ownNormalTower) weight += NORMAL_PIECE_WEIGHT;
        else if (pCurrent->piece == ownQueenTower) weight += QUEEN_TO_NORMAL_RATIO * NORMAL_PIECE_WEIGHT;
        else if (pCurrent->piece == opponentNormalTower) weight -= NORMAL_PIECE_WEIGHT;
        else if (pCurrent->piece == opponentQueenTower) weight -= QUEEN_TO_NORMAL_RATIO * NORMAL_PIECE_WEIGHT;
        pCurrent = pCurrent->next;
    }
    return weight;
}

/* Nimmt die Ursprungs- und Zielkoordinaten einer (gültigen!) Turmverschiebung entgegen und gibt ein Rating zurück,
 * wie gut dieser Zug ist. Rating wird nach folgenden Kriterien bestimmt:
 *
 * - Es gibt ein Grundrating, das jeder gültige Zug einfach hat.
 * - Ein Zug mit einem normalen Turm ist ein kleines bisschen besser als ein Zug mit einer Dame, weil es einen Schritt
 *   Richtung Damenumwandlung bedeutet.
 * - Zusätzlich wird betrachtet, wie der Zug die Gefahrenlage ändert: Wenn danach eigene Spielsteine bedroht werden,
 *   ist das schlecht --> bestrafen. Wenn danach gegnerische Spielsteine bedroht werden, ist das gut -> belohnen (aber nicht
 *   um den gleichen Faktor wie eigene Bedrohung, da der Gegner ja als nächstes dran ist und reagieren kann).
 * */
float evaluateMove(coordinate origin, coordinate target) {
    float rating = POSSIBLE_MOVE_BASE_RATING;

    if (getTopPiece(origin) == ownNormalTower) {
        rating += ADVANCE_NORMAL_PIECE_BONUS;
    }

    int piecesInDangerOwnPrev = howManyInDangerOwn();
    int piecesInDangerOppPrev = howManyInDangerOpponent();

    moveTower(origin, target);

    int piecesInDangerOwnAfter = howManyInDangerOwn();
    int dangerDifferenceOwn = piecesInDangerOwnAfter - piecesInDangerOwnPrev;
    if (dangerDifferenceOwn > 0) printf("  Mit diesem Zug bringe ich einen meiner Spielsteine in Gefahr...\n");
    else if (dangerDifferenceOwn < 0) printf("  Mit diesem Zug bringe ich einen eigenen gefährdeten Spielstein in Sicherheit.\n");
    rating -= OWN_ENDANGERMENT_FACTOR * dangerDifferenceOwn;

    int piecesInDangerOppAfter = howManyInDangerOpponent();
    int dangerDifferenceOpp = piecesInDangerOppAfter - piecesInDangerOppPrev;
    if (dangerDifferenceOpp > 0) printf("  Mit diesem Zug bedrohe ich einen gegnerischen Spielstein...\n");
    else if (dangerDifferenceOpp < 0) printf("  Mit diesem Zug höre ich auf, einen gegnerischen Spielstein zu bedrohen.\n");
    rating += THREATEN_OPPONENT_FACTOR * dangerDifferenceOpp;

    if (justConvertedToQueen) {
        printf("  Mit diesem Zug kann ich eine Damenumwandlung durchführen!\n");
        rating += CONVERT_TO_QUEEN_BONUS;
    }

    undoMoveTower(origin, target);

    return rating;
}

/* Gegeben eine Koordinate, an der ein Turm steht. Testet, an welche Felder der Turm verschoben werden kann.
 * (Im Moment nur als Kommandozeilenausgabe) */
move tryAllMoves(coordinate origin) {

    char strOrigin[3];
    char strTarget[3];
    int newX, newY;
    coordToCode(strOrigin, origin);

    move bestMove = createMoveStruct();
    float rating;

    // normaler Turm
    if (getTopPiece(origin) == 'w' || getTopPiece(origin) == 'b') {

        if (getTopPiece(origin) == 'w') newY = origin.yCoord + 1;
        else newY = origin.yCoord - 1;

        for (int round = 0; round < 2; round++) {   // 2, weil einmal nach links und einmal nach rechts
            switch (round) {
                case 0:
                    newX = origin.xCoord + 1;
                    break;
                case 1:
                    newX = origin.xCoord - 1;
                    break;
            }

            if (checkMove(origin, numsToCoord(newX, newY)) == 0) {

                coordToCode(strTarget, numsToCoord(newX, newY));
                printf("Von %s nach %s Bewegen ist ein gülter Zug.\n", strOrigin, strTarget);

                rating = evaluateMove(origin, numsToCoord(newX, newY));
                if (rating > bestMove.rating) {
                    bestMove.origin = origin;
                    bestMove.target = numsToCoord(newX, newY);
                    bestMove.rating = rating;
                }
            }
        }

    }

    // Turm mit Dame oben
    if (getTopPiece(origin) == 'W' || getTopPiece(origin) == 'B') {

        int xDirection, yDirection;
        for (int round = 0; round < 4; round++) { // 4, weil vorne links, vorne rechts, hinten links, hinten rechts
            switch (round) {
                case 0:
                    xDirection = 1;
                    yDirection = 1;
                    break;
                case 1:
                    xDirection = 1;
                    yDirection = -1;
                    break;
                case 2:
                    xDirection = -1;
                    yDirection = 1;
                    break;
                case 3:
                    xDirection = -1;
                    yDirection = -1;
                    break;
            }

            for (int i = 1; i < NUM_ROWS; i++) {
                newX = origin.xCoord + i * xDirection;
                newY = origin.yCoord + i * yDirection;

                if (checkMove(origin, numsToCoord(newX, newY)) != 0) break;

                coordToCode(strTarget, numsToCoord(newX, newY));
                printf("Von %s nach %s Bewegen ist ein gülter Zug.\n", strOrigin, strTarget);

                rating = evaluateMove(origin, numsToCoord(newX, newY));
                if (rating > bestMove.rating) {
                    bestMove.origin = origin;
                    bestMove.target = numsToCoord(newX, newY);
                    bestMove.rating = rating;
                }

                moveTower(origin, numsToCoord(newX, newY));
                undoMoveTower(origin, numsToCoord(newX, newY));
            }

        }
    }

    return bestMove;
}

// wird weiter unten implementiert, aber evaluteCapture braucht das diese Funktion schon
move tryCaptureAgain(coordinate nowBlocked, coordinate newOrigin, bool verbose);

/* Nimmt die Ursprungs- und Zielkoordinaten eines (gültigen!) Schlagzugs entgegen und gibt ein Rating zurück,
 * wie gut dieser Zug ist. Rating wird nach folgenden Kriterien bestimmt:
 *
 * - Es gibt ein Grundrating, das jeder gültige Zug einfach hat.
 * - Wenn mit diesem Schlag eine Damenumwandlung möglich ist, soll das stark belohnt werden.
 * - Das "Gewicht" des eigenen Turms soll miteinbezogen werden: Falls möglich wäre es besser, mit einem "schweren"
 *   Turm zu ziehen, der nicht so leicht vom Gegner geschlagen und damit umgedreht werden kann.
 * - Das "Gewicht" des Victim-Turms soll genauso betrachtet werden: Es ist gut, einen möglichst "leichten" Turm anzugreifen,
 *   da man hier evtl. geschlagene Steine zurückerobern kann.
 * - Wenn nach diesem Zug noch weitere Teilzüge möglich sind, soll das belohnt werden.
 * - Ansonsten soll betrachtet werden, wie der Zug die Gefahrenlage ändert: Wenn danach eigene Spielsteine bedroht werden,
 *   ist das schlecht --> bestrafen. Wenn danach gegnerische Spielsteine bedroht werden, ist das gut -> belohnen (aber nicht
 *   um den gleichen Faktor wie eigene Bedrohung, da der Gegner ja als nächstes dran ist und reagieren kann).
 * */
float evaluateCapture(coordinate origin, coordinate target, bool verbose) {
    float rating = POSSIBLE_MOVE_BASE_RATING;

    int ownWeight = computeTowerWeight(origin);
    if (verbose && ownWeight < 0) printf("  Der Turm, mit dem ich schlagen will, ist ziemlich schwach. Damit anzugreifen ist vielleicht nicht so gut.\n");
    rating += WEIGHT_RATING_FACTOR * ownWeight;

    coordinate victim = computeVictim(origin, target);
    int victimWeight = computeTowerWeight(victim);
    if (verbose && victimWeight > 0) printf("  Der gegnerische Turm ist ziemlich stark. Vielleicht sollte ich lieber einen anderen angreifen.\n");
    rating -= WEIGHT_RATING_FACTOR * victimWeight;

    if (getTopPiece(victim) == opponentQueenTower) {
        if (verbose) printf("  Mit diesem Zug schlage ich eine gegnerische Dame!\n");
        rating += CAPTURE_QUEEN_BONUS;
    }

    int piecesInDangerOwnPrev = howManyInDangerOwn();
    int piecesInDangerOppPrev = howManyInDangerOpponent();

    captureTower(origin, target);

    move potentialNextMove = tryCaptureAgain(origin, target, false);
    if (potentialNextMove.origin.xCoord != potentialNextMove.target.xCoord || potentialNextMove.origin.yCoord != potentialNextMove.target.yCoord) {
        if (verbose) printf("  Dieser Schlag ist gut, weil ich danach noch weiterschlagen kann.\n");
        rating += ANOTHER_CAPTURE_POSSIBLE_BONUS;
    } else {
        int piecesInDangerOwnAfter = howManyInDangerOwn();
        int dangerDifferenceOwn = piecesInDangerOwnAfter - piecesInDangerOwnPrev;
        if (dangerDifferenceOwn > 0) {
            if (verbose) printf("  Mit diesem Zug bringe ich einen meiner Spielsteine in Gefahr...\n");
            rating -= OWN_ENDANGERMENT_FACTOR * dangerDifferenceOwn;
        } else if (dangerDifferenceOwn < 0) {
            if (verbose) printf("  Mit diesem Zug bringe ich einen eigenen gefährdeten Spielstein in Sicherheit.\n");
            rating -= OWN_ENDANGERMENT_FACTOR * dangerDifferenceOwn; // da dangerDifference negativ ist, wirkt das als Bonus
        }
    }

    int piecesInDangerOppAfter = howManyInDangerOpponent();
    int dangerDifferenceOpp = piecesInDangerOppAfter - piecesInDangerOppPrev;
    if (dangerDifferenceOpp > 0) {
        printf("  Mit diesem Zug bedrohe ich einen gegnerischen Spielstein...\n");
        rating += THREATEN_OPPONENT_FACTOR * dangerDifferenceOpp;
    }
    else if (dangerDifferenceOpp < 0) {
        printf("  Mit diesem Zug höre ich auf, einen gegnerischen Spielstein zu bedrohen.\n");
        rating += THREATEN_OPPONENT_FACTOR * dangerDifferenceOpp;
    }


    if (justConvertedToQueen) {
        if (verbose) printf("  Mit diesem Schlag kann ich eine Damenumwandlung durchführen!\n");
        rating += CONVERT_TO_QUEEN_BONUS;
    }

    undoCaptureTower(origin, target);

    return rating;
}


/* Gegeben eine Koordinate, an der ein Turm steht, und eine zweite "blockierte" Koordinate. Testet alle Schläge
 * die der Turm an der ersten Koordinate ausführen könnte – außer solche, die zum blockierten Feld/darüber hinaus
 * führen würden. Gibt den besten möglichen Zug zurück. Wenn keine Schläge möglich sind: Gibt Standard-move mit
 * origin- und target-Koordinaten -1 und rating -FLT_MAX zurück. */
move tryAllCapturesExcept(coordinate origin, coordinate blocked, bool verbose) {

    char strOrigin[3];
    char strTarget[3];
    int newX, newY;
    coordToCode(strOrigin, origin);

    move bestMove = createMoveStruct();
    float rating;

    // normaler Turm
    if (getTopPiece(origin) == 'w' || getTopPiece(origin) == 'b') {

        for (int round = 0; round < 4; round++) { // 4, weil vier Richtungen, in die geschlagen werden könnte
            switch (round) {
                case 0:
                    newX = origin.xCoord + 2;
                    newY = origin.yCoord + 2;
                    break;
                case 1:
                    newX = origin.xCoord + 2;
                    newY = origin.yCoord - 2;
                    break;
                case 2:
                    newX = origin.xCoord - 2;
                    newY = origin.yCoord + 2;
                    break;
                case 3:
                    newX = origin.xCoord - 2;
                    newY = origin.yCoord - 2;
                    break;
            }

            if (newX == blocked.xCoord && newY == blocked.yCoord) continue;
            if (checkCapture(origin, numsToCoord(newX, newY)) == 0) {
                coordToCode(strTarget, numsToCoord(newX, newY));
                if (verbose) printf("Von %s nach %s Schlagen ist ein gülter Zug.\n", strOrigin, strTarget);

                rating = evaluateCapture(origin, numsToCoord(newX, newY), verbose);
                if (rating > bestMove.rating) {
                    bestMove.origin = origin;
                    bestMove.target = numsToCoord(newX, newY);
                    bestMove.rating = rating;
                }

            }
        }

    }

    // Turm mit Dame oben
    if (getTopPiece(origin) == 'W' || getTopPiece(origin) == 'B') {
        int xDirection;
        int yDirection;

        for (int round = 0; round < 4; round++) { // 4, weil vorne links, vorne rechts, hinten links, hinten rechts
            switch (round) {
                case 0:
                    xDirection = 1;
                    yDirection = 1;
                    break;
                case 1:
                    xDirection = 1;
                    yDirection = -1;
                    break;
                case 2:
                    xDirection = -1;
                    yDirection = 1;
                    break;
                case 3:
                    xDirection = -1;
                    yDirection = -1;
                    break;
            }

            // nur überprüfen, wenn blocked eine echte Koordinate ist (nicht der Dummy -1, -1 -> sonst wird versehentlich der Zug nach links unten blockiert)
            if (blocked.xCoord != -1 && blocked.yCoord != -1) {
                // berechne die blockierte Richtung, in diese dürfen wir nicht ziehen
                int blockedXDirection = getSign(blocked.xCoord - origin.xCoord);
                int blockedYDirection = getSign(blocked.yCoord - origin.yCoord);
                if (xDirection == blockedXDirection && yDirection == blockedYDirection) continue;
            }

            for (int i = 2; i < NUM_ROWS; i++) {
                newX = origin.xCoord + i * xDirection;
                newY = origin.yCoord + i * yDirection;

                if (checkCapture(origin, numsToCoord(newX, newY)) != 0) continue;

                coordToCode(strTarget, numsToCoord(newX, newY));
                if (verbose) printf("Von %s nach %s Schlagen ist ein gülter Zug.\n", strOrigin, strTarget);

                rating = evaluateCapture(origin, numsToCoord(newX, newY), verbose);
                if (rating > bestMove.rating) {
                    bestMove.origin = origin;
                    bestMove.target = numsToCoord(newX, newY);
                    bestMove.rating = rating;
                }

                break;
            }
        }
    }

    return bestMove;
}

/* Gegeben eine Koordinate, an der ein Turm steht. Testet alle Schläge, die der Turm an der ersten Koordinate ausführen
 * könnte, und gibt den besten davon zurück. [Spezialfall von tryAllCapturesExcept, Except wird nicht genutzt]*/
move tryAllCaptures(coordinate origin) {
    return tryAllCapturesExcept(origin, numsToCoord(-1, -1), true);
}

/* Wendet tryAllCapturesExcept an. Wenn kein gültiger Spielzug existiert, werden in move origin und target beide auf
 * die Ursprungskoordinate gesetzt. */
move tryCaptureAgain(coordinate nowBlocked, coordinate newOrigin, bool verbose) {
    move ret = tryAllCapturesExcept(newOrigin, nowBlocked, verbose);

    // wenn tryAllCaptures keinen gültigen Zug finden kann: kein Zug mehr, kodiert indem origin = target
    if (ret.origin.xCoord == -1 || ret.origin.yCoord == -1 || ret.target.xCoord == -1 || ret.target.yCoord == -1) {
        ret.origin = newOrigin;
        ret.target = newOrigin;
    }

    return ret;
}

/* Bestimmt den günstigen Spielzug auf Basis des aktuellen Spielbretts. Schreibt diesen in den angegebenen String. */
void think(char *answer) {
    move overallBestMove = createMoveStruct();
    move thisMove;
    int answerCounter = 0;

    justConvertedToQueen = false;

    // versuche erst, einen gültigen Schlag-Teilzug zu finden
    for (int i = 0; i < NUM_ROWS; i++) {
        for (int j = 0; j < NUM_ROWS; j++) {
            if (getTopPiece(numsToCoord(i, j)) != ownNormalTower && getTopPiece(numsToCoord(i,j)) != ownQueenTower) {
                continue;
            }
            thisMove = tryAllCaptures(numsToCoord(i,j));
            if (thisMove.rating > overallBestMove.rating) {
                overallBestMove.origin = thisMove.origin;
                overallBestMove.target = thisMove.target;
                overallBestMove.rating = thisMove.rating;
            }
        }
    }

    // wenn ein gültiger Schlag-Teilzug gefunden werden kann: in answer schreiben, danach in Schleife nach weiteren Teilzügen vom Zielfeld aus suchen
    if (overallBestMove.origin.xCoord != -1 && overallBestMove.origin.yCoord != -1 && overallBestMove.target.xCoord != -1 && overallBestMove.target.yCoord != -1) {
        coordToCode(answer, overallBestMove.origin);
        answerCounter += 2;
        answer[answerCounter] = ':';
        answerCounter += 1;
        coordToCode(answer + answerCounter, overallBestMove.target);
        answerCounter += 2;

        bool moreCapturesLoop = true;

        while (moreCapturesLoop) {
            printf("\nVerwende den Teilzug %s und suche weiter:\n", answer+(answerCounter-5));
            // führe den besten Zug tatsächlich aus
            captureTower(overallBestMove.origin, overallBestMove.target);
            justConvertedToQueen = false;

            overallBestMove = tryCaptureAgain(overallBestMove.origin, overallBestMove.target, true);

            if (overallBestMove.origin.xCoord == overallBestMove.target.xCoord && overallBestMove.origin.yCoord == overallBestMove.target.yCoord) {
                // kein weiteres Schlagen mehr möglich
                printf("Von hier ist kein weiterer Teilzug möglich.\n");
                moreCapturesLoop = false;
            } else {
                // weiterer Schlagzug gefunden -> in answer schreiben
                answer[answerCounter] = ':';
                answerCounter += 1;
                coordToCode(answer + answerCounter, overallBestMove.target);
                answerCounter += 2;
            }

        }
    } else { // kein Schlag-Teilzug gefunden -> suche Beweg-Zug
        for (int i = 0; i < NUM_ROWS; i++) {
            for (int j = 0; j < NUM_ROWS; j++) {
                if (getTopPiece(numsToCoord(i, j)) != ownNormalTower && getTopPiece(numsToCoord(i,j)) != ownQueenTower) {
                    continue;
                }
                thisMove = tryAllMoves(numsToCoord(i,j));
                if (thisMove.rating > overallBestMove.rating) {
                    overallBestMove.origin = thisMove.origin;
                    overallBestMove.target = thisMove.target;
                    overallBestMove.rating = thisMove.rating;
                }
            }
        }

        coordToCode(answer, overallBestMove.origin);
        answerCounter += 2;
        answer[answerCounter] = ':';
        answerCounter += 1;
        coordToCode(answer + answerCounter, overallBestMove.target);
        answerCounter += 2;
    }

    answer[answerCounter] = '\n'; // answer braucht am Ende ein Newline, sonst wird die Nachricht in mainloop_filehandler liegengelassen
}
