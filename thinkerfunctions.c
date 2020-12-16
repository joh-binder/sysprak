#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "thinkerfunctions.h"

static int towerAllocCounter = 0;

/* Setzt voraus, dass ein Speicherblock reserviert ist. Nimmt als Parameter einen Pointer auf den Anfang dieses
 * Speicherblocks und die Blockgröße; außerdem eine Wunschgröße. Gibt dann einen Pointer auf einen Abschnitt
 * des Speicherblocks in der Wunschgröße zurück, insofern genug Platz vorhanden ist. Intern wird ein Zähler
 * versetzt, sodass der nächste Funktionsaufruf einen anderen Abschnitt liefert.
 *
 * [Beachte: Pro Programmaufruf kann diese Methode nur für die Verwaltung eines (1) Speicherblocks benutzt werden.]
 */
tower *towerAlloc(tower *pointerToStart, unsigned long maxSizeOfBlock) {
    if (maxSizeOfBlock < (towerAllocCounter + 1) * sizeof(tower)) {
        fprintf(stderr, "Fehler! Speicherblock ist bereits voll. Kann keinen Speicher mehr zuteilen.\n");
        return (tower *)NULL;
    } else {
        tower *pTemp = pointerToStart + towerAllocCounter;
        towerAllocCounter += 1;
        return pTemp;
    }
}

void resetTallocCounter(void) {
    towerAllocCounter = 0;
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

/* Gegeben ein Array von Pointern auf tower (=ein Spielbrett) und eine Koordinate, gibt den zur Koordinate
 * gehörigen Pointer zurück; oder NULL bei ungültiger Koordinate. */
tower *getPointerToSquare(tower **board, coordinate c) {
    if (c.xCoord == -1 || c.yCoord == -1) {
        fprintf(stderr, "Fehler! Ungültige Koordinate.\n");
        return NULL;
    } else {
        return board[8*c.yCoord + c.xCoord];
    }
}

/* Gegeben ein Array von Pointern auf tower (=ein Spielbrett) und eine Koordinate, gibt den obersten Spielstein,
 * der auf diesem Feld liegt, als Char zurück; oder 'X' bei ungültiger Koordinate. */
char getTopPiece(tower **board, coordinate c) {
    if (getPointerToSquare(board, c) == NULL) {
        return 'X';
    } else {
        return getPointerToSquare(board, c)->piece;
    }
}

// Druckt zu einem gegebenen Spielfeld zu jedem Feld den obersten Spielstein formatiert aus.
void printTopPieces(tower **pBoard) {
    printf("   A B C D E F G H\n");
    printf(" +-----------------+\n");
    for (int i = 7; i >= 0; i--) {
        printf("%d| ", i+1);
        for (int j = 0; j < 8; j++) {
            if (getPointerToSquare(pBoard, numsToCoord(j, i)) == (tower *)NULL) {
                printf("- ");
            } else {
                printf("%c ", getTopPiece(pBoard, numsToCoord(j, i)));
            }
        }
        printf("|%d\n", i+1);
    }
    printf(" +-----------------+\n");
    printf("   A B C D E F G H\n");
}

/* Wendet printTopPieces an und druckt zusätzlich eine Liste mit allen weißen und schwarzen Türmen.*/
void printFull(tower **pBoard) {
    printTopPieces(pBoard);

    printf("\nWhite Towers\n");
    printf("============\n");
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            char topPiece = getTopPiece(pBoard, numsToCoord(i, j));
            if (topPiece == 'w' || topPiece == 'W') {
                char *towerBuffer = malloc(33 * sizeof(char));
                char *coordinateBuffer = malloc(3 * sizeof(char));
                towerToString(towerBuffer, pBoard, numsToCoord(i, j));
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
            char topPiece = getTopPiece(pBoard, numsToCoord(j, i));
            if (topPiece == 'b' || topPiece == 'B') {
                char *towerBuffer = malloc(33 * sizeof(char));
                char *coordinateBuffer = malloc(3 * sizeof(char));
                towerToString(towerBuffer, pBoard, numsToCoord(j, i));
                coordToCode(coordinateBuffer, numsToCoord(j, i));
                printf("%s: %s\n", coordinateBuffer, towerBuffer);
                free(towerBuffer);
                free(coordinateBuffer);
            }
        }
    }
}

/* Erzeugt einen neuen tower/Spielstein und setzt ihn auf das Spielbrett. Verlangt dazu als Parameter das
 * Spielbrett, eine Zielkoordinate, die Art des Spielsteins (als Char) und einen Pointer auf den
 * Anfangsbereich des (Gesamt-)Speicherblocks, in dem der neue tower gespeichert wird. */
void addToSquare(tower **board, coordinate c, char piece, tower *pointerToAlloc, int maxTowersNeeded) {
    if (c.xCoord == -1 || c.yCoord == -1) {
        fprintf(stderr, "Fehler! Ungültige Koordinaten.\n");
    } else {
        tower *pNewTower;
        pNewTower = towerAlloc(pointerToAlloc, maxTowersNeeded * sizeof(tower));

        pNewTower->piece = piece;
        pNewTower->next = getPointerToSquare(board, c);
        board[8*c.yCoord + c.xCoord] = pNewTower;
    }
}

/* Verschiebt auf einem gegebenen Array von Pointern auf tower (=einem Spielbrett) den Turm, der an einer
 * gegebenen Koordinate steht, an eine andere Koordinate. Druckt eine Fehlermeldung, wenn das Ursprungsfeld
 * leer oder das Zielfeld schon besetzt ist. Alle anderen Züge werden akzeptiert (z.B. auch A1 -> H7),
 * d.h. es muss beachtet werden, dass nur regelkonforme Züge an die Funktion übergeben werden.
 * Gibt 0 bei Erfolg und -1 bei Fehler zurück. */
int moveTower(tower **board, coordinate origin, coordinate target) {
    if (getPointerToSquare(board, origin) == NULL) {
        char code[3];
        coordToCode(code, origin);
        fprintf(stderr, "Fehler! Auf dem Ursprungsfeld %s liegt liegt gar kein Stein.\n", code);
        return -1;
    } else if (getPointerToSquare(board, target) != NULL) {
        char code[3];
        coordToCode(code, target);
        fprintf(stderr, "Fehler! Auf dem Zielfeld %s liegt schon ein Stein.\n", code);
        return -1;
    } else {
        board[8*target.yCoord + target.xCoord] = getPointerToSquare(board, origin);
        board[8*origin.yCoord + origin.xCoord] = NULL;
        return 0;
    }
}

/* Macht eine moveTower-Operation rückgängig. Die Parameterreihenfolge bleibt gleich.
 * Gibt 0 bei Erfolg und -1 bei Fehler zurück. */
int undoMoveTower(tower **board, coordinate origin, coordinate target) {
    return moveTower(board, target, origin);
}

/* Auf einem gegebenen Array von Pointern auf tower (=einem Spielbrett), nimmt den Turm, der an der
 * Koordinate origin steht, schlägt den Turm, der an victim steht, (d.h. entfernt den obersten Spielstein und
 * fügt ihn zum eigenen Turm hinzu) und setzt den Ergebnisturm dann an die Koordinate target.
 *
 * Gibt eine Fehlermeldung aus, wenn origin oder victim leer sind oder target schon besetzt ist,
 * ansonsten wird jeder Zug ausgeführt. Es muss also selbst sichergestellt werden, dass die übergebenen Züge
 * regelkonform sind.
 * Gibt 0 bei Erfolg und -1 bei Fehler zurück. */
int beatTower(tower **board, coordinate origin, coordinate victim, coordinate target) {
    if ((origin.xCoord == victim.xCoord && origin.yCoord == victim.yCoord) || (victim.xCoord == target.xCoord && victim.yCoord == target.yCoord) || (origin.xCoord == target.xCoord && origin.yCoord == target.yCoord)) {
        fprintf(stderr, "Fehler! Zwei der Koordinaten sind gleich.\n");
        return -1;
    }

    if (getPointerToSquare(board, origin) == NULL) {
        char code[3];
        coordToCode(code, origin);
        fprintf(stderr, "Fehler! Auf dem Ursprungsfeld %s liegt gar kein Stein.\n", code);
        return -1;
    } else if (getPointerToSquare(board, victim) == NULL) {
        char code[3];
        coordToCode(code, victim);
        fprintf(stderr, "Fehler! Auf dem Feld %s ist gar kein Stein, der geschlagen werden könnte.\n", code);
        return -1;
    } else if (getPointerToSquare(board, target) != NULL) {
        char code[3];
        coordToCode(code, target);
        fprintf(stderr, "Fehler! Auf dem Zielfeld %s liegt schon ein Turm.\n", code);
        return -1;
    }

    // überprüft, ob auf den Feldern zwischen Origin und Victim etwas steht
    int xDirection = getSign(victim.xCoord-origin.xCoord);
    int yDirection = getSign(victim.yCoord-origin.yCoord);
    int i = origin.xCoord + xDirection;
    int j = origin.xCoord + yDirection;
    while (i != victim.xCoord) {
        if (getPointerToSquare(board, numsToCoord(i, j)) != NULL) {
            char code[3];
            coordToCode(code, numsToCoord(i,j));
            fprintf(stderr, "Fehler! Auf dem Feld %s zwischen Origin und Victim steht ein Turm. Schlagen ist deshalb nicht möglich.\n", code);
            return -1;
        }
    }

    // überprüft, ob Target diagonal hinter dem Feld Victim liegt
    if (target.xCoord != victim.xCoord + xDirection || target.yCoord != victim.yCoord + yDirection) {
        fprintf(stderr, "Fehler! Schlagen ist nur möglich, wenn das Feld Target diagonal hinter dem Feld Victim liegt.\n");
        return -1;
    }

    // ab hier tatsächliche Versetzung der Spielsteine
    tower *pNewVictimTower = getPointerToSquare(board, victim)->next;
    tower *pTowerForTarget = getPointerToSquare(board, origin);

    tower *pCurrent = pTowerForTarget;
    while (pCurrent->next != NULL) {
        pCurrent = pCurrent->next;
    }
    pCurrent->next = getPointerToSquare(board, victim);
    pCurrent->next->next = NULL;

    board[8*victim.yCoord + victim.xCoord] = pNewVictimTower;
    board[8*target.yCoord + target.xCoord] = pTowerForTarget;
    board[8*origin.yCoord + origin.xCoord] = NULL;
    return 0;

}

int undoBeatTower(tower **board, coordinate origin, coordinate victim, coordinate target) {
    if (getPointerToSquare(board, origin) != NULL) {
        char code[3];
        coordToCode(code, origin);
        fprintf(stderr, "Fehler! Auf dem Ursprungsfeld %s liegt schon ein Turm. In beatTower hätte dieser aber weggezogen werden müssen.\n", code);
        return -1;
    } else if (getPointerToSquare(board, target) == NULL) {
        char code[3];
        coordToCode(code, target);
        fprintf(stderr, "Fehler! Auf dem Zielfeld %s liegt gar kein Stein. In beatTower hätte hier aber der neue Turm hingestellt werden müssen.\n", code);
        return -1;
    } else {
        tower *pCurrent = getPointerToSquare(board, target);
        tower *pBottomPieceOfTarget;
        while (pCurrent->next->next != NULL) {
            pCurrent = pCurrent->next;
        }
        pBottomPieceOfTarget = pCurrent->next;
        pCurrent->next = NULL;

        pBottomPieceOfTarget->next = getPointerToSquare(board, victim);
        board[8*victim.yCoord+victim.xCoord] = pBottomPieceOfTarget;

        board[8*origin.yCoord+origin.xCoord] = getPointerToSquare(board, target);
        board[8*target.yCoord+target.xCoord] = NULL;

        return 0;
    }

}

/* Nimmt als Parameter einen String (sollte 33 Chars aufnehmen können), ein Spielbrett und eine Koordinate.
 * Repräsentiert alle Spielsteine, aus denen der Turm an der angegebenen Kooordinate besteht, als Folge von
 * Buchstaben (z.B. Bwwbw) und schreibt das Ergebnis in den String.
 */
void towerToString(char *target, tower **board, coordinate c) {
    char ret[32+1];
    tower *pCurrent = getPointerToSquare(board, c);

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