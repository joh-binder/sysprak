#include <float.h>
#include "thinkerfunctions.h"

#define NUMBER_OF_PIECES_IN_BASHNI 32
#define NUM_ROWS 8 // (auch Zahl der Spalten)
#define COORDINATE_LENGTH 2 // ein Buchstabe und eine Zahl
#define POSSIBLE_DIRECTIONS_FORWARD 2 // nach vorne links oder vorne rechts
#define POSSIBLE_DIRECTIONS_FORWARD_AND_BACKWARD 4 // vorne links, vorne rechts, hinten links, hinten rechts
#define DISTANCE_FOR_VALID_CAPTURE 2 // um zu schlagen, müssen Türme (bei Dame mindestens) zwei Fehler entfernt sein

// Parameter für die Bewertungsfunktionen (haben keine richtige theoretische Grundlage -> rumprobieren, was gut klappt)
#define QUEEN_TO_NORMAL_RATIO 3.0
#define NORMAL_PIECE_WEIGHT 1.0
#define POSSIBLE_MOVE_BASE_RATING 1.0
#define ADVANCE_NORMAL_PIECE_BONUS 0.6
#define CONVERT_TO_QUEEN_BONUS 3.5
#define OWN_ENDANGERMENT_FACTOR 0.9
#define THREATEN_OPPONENT_FACTOR 0.2
#define CAPTURE_QUEEN_BONUS 3.5
#define ANOTHER_CAPTURE_POSSIBLE_BONUS 1.75
#define WEIGHT_RATING_FACTOR 0.6

static int towerAllocCounter = 0;
static unsigned int sizeOfTowerMalloc;
static tower *pPieces;
static tower **pBoard;

static char ownNormalTower, ownQueenTower, opponentNormalTower, opponentQueenTower;
static bool justConvertedToQueen = false;

static int shmidMoves = -1;
static struct gameInfo *pGeneralInfo;
static struct line *pMoveInfo;

/* Wandelt einen Buchstabe-Zahl-Code in den Datentyp coordinate um.
 * Bei ungültigen Koordinaten (alles außer A-H und 1-8) sind die Koordinaten -1, -1. */
coordinate codeToCoord(char code[COORDINATE_LENGTH]) {
    coordinate ret;
    switch (code[0]) {
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
    switch (code[1]) {
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
    if (x >= 0 && x <= NUM_ROWS - 1) { ret.xCoord = x; }
    else { ret.xCoord = -1; }
    if (y >= 0 && y <= NUM_ROWS - 1) { ret.yCoord = y; }
    else { ret.yCoord = -1; }
    return ret;
}

/* Nimmt als Parameter einen String (sollte 3 Chars aufnehmen können) und eine coordinate. Wandelt die Koordinate um
 * in die Buchstabe-Zahl-Repräsentation (z.B. x = 4, y = 2 --> B3) und schreibt das Ergebnis in den String */
void coordToCode(char *target, coordinate c) {
    char ret[COORDINATE_LENGTH + 1];

    switch (c.xCoord) {
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
    switch (c.yCoord) {
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

    ret[COORDINATE_LENGTH] = '\0';
    strncpy(target, ret, COORDINATE_LENGTH + 1);
}

// erzeugt ein neues struct gameInfo und initialisiert es mit Standardwerten
move createMoveStruct(void) {
    move ret;
    ret.origin = numsToCoord(-1, -1);
    ret.target = numsToCoord(-1, -1);
    ret.rating = -FLT_MAX;
    return ret;
}

/* Mit dieser Funktion muss die Spielernummer an thinkerfunctions übergeben werden,
 * damit hier bekannt ist, was die eigene Farbe und was die des Gegners ist. */
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
int setUpMemoryForThinker(struct gameInfo *pG) {
    pGeneralInfo = pG;

    if (setUpWhoIsWho(pGeneralInfo->ownPlayerNumber) != 0) {
        return -1;
    }

    // Shared-Memory für die Spielzüge aufrufen
    shmidMoves = accessExistingMoveShmem();
    if (shmidMoves == -1) {
        fprintf(stderr, "Fehler beim Zugriff im Thinker auf das Shared Memory für die Spielzüge.\n");
        return -1;
    }
    pMoveInfo = shmAttach(shmidMoves);
    if (pMoveInfo == NULL) {
        fprintf(stderr, "Fehler beim Versuch, im Thinker das Shmemory-Segment für die Spielzüge anzubinden.\n");
        return -1;
    }

    // genug Speicherplatz für das Spielbrett freigeben
    pBoard = malloc(sizeof(tower *) * NUM_ROWS * NUM_ROWS);
    if (pBoard == NULL) {
        fprintf(stderr, "Fehler beim Anfordern von Speicherplatz für das Spielbrett.\n");
        return -1;
    }

    // genug Speicherplatz für alle Spielsteine freigeben
    sizeOfTowerMalloc = pGeneralInfo->sizeMoveShmem * sizeof(tower);
    pPieces = malloc(sizeOfTowerMalloc);
    if (pPieces == NULL) {
        fprintf(stderr, "Fehler beim Anfordern von Speicherplatz für die Spielsteine.\n");
        return -1;
    }

    return 0;
}

/* Setzt voraus, dass schon ein Speicherblock für die Türme reserviert ist. Gibt einen Pointer auf einen Abschnitt des
 * Speicherblocks zurück, der genau groß genug für einen tower ist. Intern wird ein Zähler versetzt, sodass der
 * nächste Funktionsaufruf den darauf folgenden Abschnitt liefert. Gibt im Fehlerfall NULL zurück.*/
tower *towerAlloc(void) {
    if (sizeOfTowerMalloc < (towerAllocCounter + 1) * sizeof(tower)) {
        fprintf(stderr, "Fehler! Speicherblock ist bereits voll. Kann keinen Speicher mehr zuteilen.\n");
        return (tower *) NULL;
    } else {
        tower *pTemp = pPieces + towerAllocCounter;
        towerAllocCounter += 1;
        return pTemp;
    }
}

/* Setzt den Zähler, den towerAlloc verwendet wieder zurück.
 * Zu verwenden, wenn (in einer neuen Iteration) wieder alle Türme auf das Spielbrett gesetzt werden sollen. */
void resetTallocCounter(void) {
    towerAllocCounter = 0;
}

// Setzt alle Pointer des Spielbretts auf NULL zurück.
void resetBoard(void) {
    for (int i = 0; i < NUM_ROWS * NUM_ROWS; i++) {
        pBoard[i] = NULL;
    }
}

// Bündelt die Funktionen resetTallocCounter und resetBoard, da diese sowieso miteinander verwendet werden sollen.
void prepareNewRound(void) {
    resetTallocCounter();
    resetBoard();
}

// Gibt zu einer Koordinate auf dem Spielfeld den zugehörigen Pointer zurück; oder NULL bei ungültiger Koordinate.
tower *getPointerToSquare(coordinate c) {
    if (c.xCoord == -1 || c.yCoord == -1) {
        fprintf(stderr, "Fehler! Ungültige Koordinate.\n");
        return NULL;
    } else {
        return pBoard[NUM_ROWS * c.yCoord + c.xCoord];
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

/* Nimmt als Parameter einen String (sollte NUMBER_OF_PIECES_IN_BASHNI+1 Chars aufnehmen können) und eine Koordinate.
 * Repräsentiert alle Spielsteine, aus denen der Turm an der angegebenen Kooordinate besteht, als Folge von Buchstaben,
 * z.B. Bwwbw, und schreibt das Ergebnis in den String. Schreibt "ERROR\n" bei ungültiger Koordinate.
 * (Beachte: Bei uns werden die Steine im Turm von oben nach unten durchgegangen/gedruckt) */
void towerToString(char *target, coordinate c) {
    tower *pCurrent = getPointerToSquare(c); // Pointer auf den aktuellen Turm (anfangs der komplette Turm)

    if (pCurrent == NULL) {
        fprintf(stderr, "Fehler! Zu dieser Koordinate konnte kein Turm gefunden werden.\n");
        strncpy(target, "ERROR\n", NUMBER_OF_PIECES_IN_BASHNI + 1);
    } else {
        char ret[NUMBER_OF_PIECES_IN_BASHNI + 1]; // Ergebnisstring
        memset(ret, 0, NUMBER_OF_PIECES_IN_BASHNI + 1);

        int i = 0;
        while (i < NUMBER_OF_PIECES_IN_BASHNI) {
            if (pCurrent == NULL) { break; }
            ret[i] = pCurrent->piece; // obersten Spielstein dieses Turms in Ergebnisstring schreiben
            pCurrent = pCurrent->next; // danach ist aktueller Turm nur noch alle darunterliegenden Steine
            i++;
        }

        strncpy(target, ret, NUMBER_OF_PIECES_IN_BASHNI + 1);
    }
}

// Druckt das Spielbrett aus, sodass man jeweils den obersten Spielstein jedes Turms sehen kann.
void printTopPieces(void) {
    printf("   A B C D E F G H\n");
    printf(" +-----------------+\n");
    for (int i = NUM_ROWS - 1; i >= 0; i--) {
        printf("%d| ", i + 1);
        for (int j = 0; j < NUM_ROWS; j++) {
            if (getPointerToSquare(numsToCoord(j, i)) == (tower *) NULL) {
                if (j % 2 == i % 2) { printf("_ "); }
                else { printf(". "); }
            } else {
                printf("%c ", getTopPiece(numsToCoord(j, i)));
            }
        }
        printf("|%d\n", i + 1);
    }
    printf(" +-----------------+\n");
    printf("   A B C D E F G H\n");
}

// Wendet printTopPieces an und druckt zusätzlich je eine Liste mit allen weißen und schwarzen Türmen.
void printFull(void) {
    printTopPieces();

    printf("\nWhite Towers\n");
    printf("============\n");
    for (int i = 0; i < NUM_ROWS; i++) {
        for (int j = 0; j < NUM_ROWS; j++) {
            char topPiece = getTopPiece(numsToCoord(i, j));
            if (topPiece == 'w' || topPiece == 'W') {
                char coordinateBuffer[COORDINATE_LENGTH + 1];
                char towerBuffer[NUMBER_OF_PIECES_IN_BASHNI + 1];
                coordToCode(coordinateBuffer, numsToCoord(i, j));
                towerToString(towerBuffer, numsToCoord(i, j));
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
                char coordinateBuffer[COORDINATE_LENGTH + 1];
                char towerBuffer[NUMBER_OF_PIECES_IN_BASHNI + 1];
                coordToCode(coordinateBuffer, numsToCoord(j, i));
                towerToString(towerBuffer, numsToCoord(j, i));
                printf("%s: %s\n", coordinateBuffer, towerBuffer);
            }
        }
    }
    printf("\n");
}

/* Erzeugt einen neuen tower/Spielstein und setzt ihn auf das Spielbrett. Verlangt dazu als Parameter eine
 * Zielkoordinate und die Art des Spielsteins als Char. Gibt im Fehlerfall -1, sonst 0 zurück.
 * (Der neuerstellte Tower besteht aus dem angegebenen Spielstein und einem Pointer auf das, was vorher schon auf dieser
 * Koordinate stand: NULL/nichts oder ein anderer Tower --> hiermit lassen sich auch gestapelte Türme aufbauen)
 *
 * addToSquare() stellt sicher, dass keine ungültigen Spielsteine (alles au0er w, W, b, B) auf das Brett gestellt werden.
 * Da alle weiteren Methoden nur diese Steine verwenden, wird dort nicht nochmal auf Ungültigkeit geprüft. */
int addToSquare(coordinate c, char piece) {
    if (c.xCoord < 0 || c.xCoord >= NUM_ROWS || c.yCoord == -1 || c.yCoord >= NUM_ROWS) {
        fprintf(stderr, "Fehler! Ungültige Koordinaten.\n");
        return -1;
    } else if (!(piece == 'w' || piece == 'W' || piece == 'b' || piece == 'B')) {
        fprintf(stderr, "Fehler! %c ist gar kein gültiger Spielstein...\n", piece);
        return -1;
    } else {
        tower *pNewTower;
        pNewTower = towerAlloc();
        if (pNewTower == (tower *) NULL) { return -1; }

        pNewTower->piece = piece;
        pNewTower->next = getPointerToSquare(c);

        pBoard[NUM_ROWS * c.yCoord + c.xCoord] = pNewTower;
        return 0;
    }
}

/* Liest Spielsteine aus dem Shmemory und setzt sie auf das Spielbrett. Gibt im Fehlerfall -1 zurück, sonst 0. */
int placePiecesOnBoard(void) {
    for (int i = 0; i < pGeneralInfo->sizeMoveShmem; i++) {
        if (addToSquare(codeToCoord(pMoveInfo[i].line + 2), pMoveInfo[i].line[0]) != 0) {
            return -1;
        }
    }
    return 0;
}

/* Gegeben eine Ursprungs- und eine Zielkoordinate, überprüft ob es erlaubt wäre, den Turm so zu bewegen.
 * Wenn ja, gibt 0 zurück; wenn nein, gibt -1 zurück. */
int checkMove(coordinate origin, coordinate target) {
    if (origin.xCoord == -1 || origin.yCoord == -1) {
        return -1; // ungültige Ursprungskoordinate
    } else if (target.xCoord == -1 || target.yCoord == -1) {
        return -1; // ungültige Zielkoordinate
    } else if (origin.xCoord == target.xCoord && origin.yCoord == target.yCoord) {
        return -1; // Ziel- und Ursprungskoordiante gleich
    } else if (abs(origin.xCoord - target.xCoord) != abs(origin.yCoord - target.yCoord)) {
        return -1; // Ziel- und Ursprungskoordinate liegen nicht auf einer Diagonale
    } else if (getPointerToSquare(origin) == NULL) {
        return -1; // Ursprungsfeld ist leer
    } else if (getPointerToSquare(target) != NULL) {
        return -1; // Zielfeld ist NICHT leer
    } else {
        char originTopPiece = getTopPiece(origin);
        int originTargetSignedYDistance = target.yCoord - origin.yCoord;
        if (originTopPiece == 'w' && originTargetSignedYDistance != 1) {
            return -1; // ein normaler weißer Turm darf nur genau ein Feld diagonal vorwärts (in positive Y-Richtung) ziehen
        } else if (originTopPiece == 'b' && originTargetSignedYDistance != -1) {
            return -1; // ein normaler schwarzer Turm darf nur genau ein Feld diagonal vorwärts (in negative Y-Richtung) ziehen
        }
    }

    // überprüft alle Felder zwischen Origin und Victim
    int xDirection = getSign(target.xCoord - origin.xCoord);
    int yDirection = getSign(target.yCoord - origin.yCoord);
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
 * deren Regelkonformität vorher mit checkMove() geprüft wurden. */
void moveTower(coordinate origin, coordinate target) {
    // Spielsteine umsetzen
    pBoard[NUM_ROWS * target.yCoord + target.xCoord] = getPointerToSquare(origin);
    pBoard[NUM_ROWS * origin.yCoord + origin.xCoord] = NULL;

    // evtl. Verwandlung in Dame
    if (getTopPiece(target) == 'w' && target.yCoord == NUM_ROWS - 1) {
        getPointerToSquare(target)->piece = 'W';
        justConvertedToQueen = true;
    } else if (getTopPiece(target) == 'b' && target.yCoord == 0) {
        getPointerToSquare(target)->piece = 'B';
        justConvertedToQueen = true;
    }
}

/* Macht eine moveTower-Operation rückgängig. Die Parameterreihenfolge bleibt gleich. Sollte nicht eigenständig
 * aufgerufen werden, sondern nur in Folge einer gültigen moveTower-Operation. */
void undoMoveTower(coordinate oldOrigin, coordinate oldTarget) {
    // evtl. Damenumwandlung rückgängig machen
    if (justConvertedToQueen && getTopPiece(oldTarget) == 'W') {
        getPointerToSquare(oldTarget)->piece = 'w';
        justConvertedToQueen = false;
    } else if (justConvertedToQueen && getTopPiece(oldTarget) == 'B') {
        getPointerToSquare(oldTarget)->piece = 'b';
        justConvertedToQueen = false;
    }
    // jetzt einfach moveTower() umgekehrt verwenden
    moveTower(oldTarget, oldOrigin);
}

/* Gegeben die Koordinaten für Origin und Target, berechnet die Koordinaten für Victim
 * (d.h. ein Feld vor Target aus der Richtung, in der Origin steht).
 * Liefert vermutlich Blödsinn, wenn origin und target nicht auf einer Diagonale stehen -> vorher prüfen. */
coordinate computeVictim(coordinate origin, coordinate target) {
    int xDirection = getSign(target.xCoord - origin.xCoord);
    int yDirection = getSign(target.yCoord - origin.yCoord);
    coordinate victim;
    victim.xCoord = target.xCoord - xDirection;
    victim.yCoord = target.yCoord - yDirection;
    return victim;
}

/* Gegeben eine Ursprungs- und eine Zielkoordinate, überprüft ob es erlaubt wäre, mit dem Turm so zu schlagen.
 * Wenn ja, gibt 0 zurück; wenn nein, gibt -1 zurück. (Beachte: Target ist nicht das Feld des Turms, der geschlagen wird,
 * sondern das Feld dahinter, auf dem der eigene Turm abgestellt werden würde.)  */
int checkCapture(coordinate origin, coordinate target) {
    if (origin.xCoord == -1 || origin.yCoord == -1) {
        return -1; // ungültige Ursprungskoordinate
    } else if (target.xCoord == -1 || target.yCoord == -1) {
        return -1; // ungültige Zielkoordinate
    } else if (origin.xCoord == target.xCoord && origin.yCoord == target.yCoord) {
        return -1; // Ursprungs- und Zielfeld sind gleich
    } else if (abs(origin.xCoord - target.xCoord) != abs(origin.yCoord - target.yCoord)) {
        return -1; // Ursprungs- und Zielfeld liegen nicht auf einer Diagonale
    }

    if (getPointerToSquare(origin) == NULL) {
        return -1; // auf dem Ursprungsfeld steht gar kein Turm, mit dem man schlagen könnte
    }
    char originTopPiece = getTopPiece(origin);
    int originTargetDistance = abs(origin.xCoord - target.xCoord);
    if ((originTopPiece == 'b' || originTopPiece == 'w') && originTargetDistance != DISTANCE_FOR_VALID_CAPTURE) {
        return -1; // Um mit einem normalen Turm zu schlagen, müssen Ursprungs- und Zielfeld diagonal genau zwei Felder entfernt sein.
    } else if ((originTopPiece == 'B' || originTopPiece == 'W') && originTargetDistance < DISTANCE_FOR_VALID_CAPTURE) {
        return -1; // Um mit einer Dame zu schlagen, müssen Ursprungs- und Zielfeld diagonal mindestens zwei Felder entfernt sein.
    }

    coordinate victim = computeVictim(origin, target);
    char victimTopPiece = getTopPiece(victim);

    if (getPointerToSquare(origin) == NULL) {
        return -1; // auf dem Ursprungsfeld steht gar kein Turm, mit dem man schlagen könnte
    } else if (getPointerToSquare(victim) == NULL) {
        return -1; // auf dem Victim-Feld ist gar kein Turm, der geschlagen werden könnte
    } else if (getPointerToSquare(target) != NULL) {
        return -1; // auf dem Zielfeld steht schon ein Turm -> Abstellen nicht möglich
    }

    if ((originTopPiece == 'b' || originTopPiece == 'B') && (victimTopPiece == 'b' || victimTopPiece == 'B')) {
        return -1; // Ein schwarzer Turm kann keinen anderen schwarzen Turm schlagen.
    } else if ((originTopPiece == 'w' || originTopPiece == 'W') && (victimTopPiece == 'w' || victimTopPiece == 'W')) {
        return -1; // Ein weißer Turm kann keinen anderen weißen Turm schlagen.
    }

    // überprüft alle Felder zwischen Origin und Victim
    int xDirection = getSign(victim.xCoord - origin.xCoord);
    int yDirection = getSign(victim.yCoord - origin.yCoord);
    int i = origin.xCoord + xDirection;
    int j = origin.yCoord + yDirection;
    while (i != victim.xCoord) {
        if (getPointerToSquare(numsToCoord(i, j)) != NULL) {
            return -1; // Auf einem Feld zwischen Origin und Victim steht schon ein Turm
        }
        i += xDirection;
        j += yDirection;
    }

    return 0;
}

/* Gegeben eine Koordinate auf dem Spielbrett, überprüft ob Turm, der auf diesem Feld steht, in Gefahr ist,
 * geschlagen zu werden. Funktioniert nur für Türme in der eigenen Farbe. */
bool isInDanger(coordinate square) {
    // überprüfe alle vier Richtungen
    int xDirection, yDirection;
    for (int round = 0; round < POSSIBLE_DIRECTIONS_FORWARD_AND_BACKWARD; round++) {
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

        // berechne das Feld hinter square (wo gegnerischer Turm nach dem Schlagen von square abgestellt werden müsste)
        coordinate behind = numsToCoord(square.xCoord + xDirection, square.yCoord + yDirection);

        if (behind.xCoord == -1 || behind.yCoord == -1) {
            continue; // wenn hinter square kein gültiges Feld mehr ist -> in dieser Richtung sicher
        } else if (getPointerToSquare(behind) != NULL) {
            continue; // wenn Feld hinter square besetzt -> auch sicher
        }

        // betrachte jetzt die Gegenrichtung, also ob VOR square überhaupt ein Stein ist, der schlagen könnte
        xDirection *= -1;
        yDirection *= -1;
        coordinate potentialOpponentPiece;

        // Schleife bis 6 (bzw. NUM_ROWS-1-1), weil Bashni insgesamt 8 Felder pro Diagonale hat
        // mit 1 Feld für square und 1 für dahinter bleiben noch 6 Felder, an denen ein schlagender Stein stehen könnte
        for (int i = 1; i <= NUM_ROWS - 1 - 1; i++) {
            potentialOpponentPiece = numsToCoord(square.xCoord + i * xDirection, square.yCoord + i * yDirection);

            if (potentialOpponentPiece.xCoord == -1 || potentialOpponentPiece.yCoord == -1) {
                break; // wenn dieses Feld ungültige Koordinate -> schon über Spielbrettrand -> in dieser Richtung sicher
            } else if (getPointerToSquare(potentialOpponentPiece) == NULL) {
                continue; // wenn auf diesem Feld nichts steht -> nächste Iteration, prüfe das Feld dahinter
            } else if (getTopPiece(potentialOpponentPiece) == ownNormalTower ||
                       getTopPiece(potentialOpponentPiece) == ownQueenTower) {
                break; // wenn auf diesem Feld ein eigener Turm steht -> in dieser Richtung sicher
            } else if (checkCapture(potentialOpponentPiece, behind) == 0) {
                return true; // wenn potentialOpponentPiece schlagen kann -> gefährdet, also true
            }
        }

    }
    return false;
}

/* Zählt, wie viele Türme der eigenen Farbe sich nach Stand des momentanen Spielbretts in Gefahr befinden.
 * Eine Dame zählt mehrfach. */
int howManyInDangerOwn(void) {
    int countTotalInDanger = 0;

    for (int i = 0; i < NUM_ROWS; i++) {
        for (int j = 0; j < NUM_ROWS; j++) { // Iteration über alle Spielfelder
            coordinate thisCoord = numsToCoord(i, j);
            char topPieceHere = getTopPiece(thisCoord);
            if (topPieceHere != ownNormalTower && topPieceHere != ownQueenTower) {
                continue; // überspringen, wenn auf Spielfeld kein eigener Turm steht
            }
            if (isInDanger(thisCoord)) {
                if (topPieceHere == ownNormalTower) {
                    countTotalInDanger += 1;
                } else {
                    countTotalInDanger += QUEEN_TO_NORMAL_RATIO * 1; // Dame zählt mehrfach
                }
            }
        }
    }
    return countTotalInDanger;
}

/* Vertauscht die Information, welches die eigene und welches die gegnerische Farbe ist, in der globalen Variable.
 * Damit lässt sich isInDanger bzw. howManyInDangerOwn aus der Sicht des Gegners anwenden.
 * Nicht vergessen, dass die Farben danach wieder zurückgetauscht werden müssen! */
void swapOwnAndOppPieces(void) {
    char tempNormalTower, tempQueenTower;
    tempNormalTower = ownNormalTower;
    tempQueenTower = ownQueenTower;
    ownNormalTower = opponentNormalTower;
    ownQueenTower = opponentQueenTower;
    opponentNormalTower = tempNormalTower;
    opponentQueenTower = tempQueenTower;
}

/* Zählt, wie viele Türme der gegnerischen Farbe sich nach Stand des momentanen Spielbretts in Gefahr befinden.
 * Eine Dame zählt mehrfach. */
int howManyInDangerOpponent(void) {
    swapOwnAndOppPieces();
    int ret = howManyInDangerOwn();
    swapOwnAndOppPieces();
    return ret;
}

/* Nimmt den Turm, der an der Koordinate origin steht, schlägt den Turm, vor dem Feld target steht,
 * (d.h. entfernt den obersten Spielstein und fügt ihn zum eigenen Turm hinzu) und setzt den Ergebnisturm dann an die
 * Koordinate target. Dieser Zug wird, soweit möglich, ausgeführt, ohne Rücksicht auf Gültigkeit zu nehmen.
 * Es sollten also nur Koordinaten verwendet werden, deren Regelkonformität vorher mit checkCapture() geprüft wurden. */
void captureTower(coordinate origin, coordinate target) {
    coordinate victim = computeVictim(origin, target);

    // erstelle Pointer für die neuen Türme
    tower *pNewVictimTower = getPointerToSquare(victim)->next; // neuer Victim-Turm ist alter Victim, bis auf den obersten Spielstein
    tower *pNewTargetTower = getPointerToSquare(origin); // neuer Target-Turm ist erstmal nur der alte Origin-Turm

    // durchgehen bis zum untersten Spielstein des neuen Target-Turms (= bisher noch Origin-Turm)
    tower *pCurrent = pNewTargetTower;
    while (pCurrent->next != NULL) {
        pCurrent = pCurrent->next;
    }

    pCurrent->next = getPointerToSquare(victim); // fügt den obersten Spielstein von Victim an das untere Ende des neuen Target-Turms hinzu
    pCurrent->next->next = NULL; // next-Pointer auf NULL setzen, sonst würde man den kompletten Victim-Turm hinzufügen

    // die neuen Türme aufs Brett setzen
    pBoard[NUM_ROWS * victim.yCoord + victim.xCoord] = pNewVictimTower;
    pBoard[NUM_ROWS * target.yCoord + target.xCoord] = pNewTargetTower;
    pBoard[NUM_ROWS * origin.yCoord + origin.xCoord] = NULL;

    // evtl. Verwandlung in Dame
    if (getTopPiece(target) == 'w' && target.yCoord == NUM_ROWS - 1) {
        getPointerToSquare(target)->piece = 'W';
        justConvertedToQueen = true;
    } else if (getTopPiece(target) == 'b' && target.yCoord == 0) {
        getPointerToSquare(target)->piece = 'B';
        justConvertedToQueen = true;
    }

}

/* Macht eine captureTower()-Operation rückgängig. Die Parameterreihenfolge bleibt gleich. Es wird angenommen, dass die
 * Funktion nicht eigenständig aufgerufen wird, sondern nur in Folge einer gültigen captureTower()-Operation. */
void undoCaptureTower(coordinate oldOrigin, coordinate oldTarget) {
    coordinate victim = computeVictim(oldOrigin, oldTarget);

    // evtl. Damenumwandlung rückgängig machen
    if (getTopPiece(oldTarget) == 'W' && justConvertedToQueen) {
        getPointerToSquare(oldTarget)->piece = 'w';
        justConvertedToQueen = false;
    } else if (getTopPiece(oldTarget) == 'B' && justConvertedToQueen) {
        getPointerToSquare(oldTarget)->piece = 'b';
        justConvertedToQueen = false;
    }

    // ab hier eigentliches Umversetzen
    tower *pCurrent = getPointerToSquare(oldTarget);
    tower *pBottomPieceOfTarget;
    while (pCurrent->next->next != NULL) {
        pCurrent = pCurrent->next; // durchgehen, bis pCurrent auf den zweituntersten Spielstein von oldTarget zeigt
    }

    pBottomPieceOfTarget = pCurrent->next; // so bekommen wir den untersten Spielstein von Target
    pCurrent->next = NULL; // indem der next-Pointer auf NULL gesetzt wird, wird der unterste Spielstein vom Target-Turm entfernt

    pBottomPieceOfTarget->next = getPointerToSquare(victim); // an den untersten Spielstein von Target wird der ganze Victim-Turm angefügt -> rekonstruiert den Victim-Turm, wie er vorher war
    pBoard[NUM_ROWS * victim.yCoord + victim.xCoord] = pBottomPieceOfTarget; // rekonstruierter Victim-Turm auf Feld Victim stellen

    pBoard[NUM_ROWS * oldOrigin.yCoord + oldOrigin.xCoord] = getPointerToSquare(oldTarget); // der restliche Turm auf Target kommt zurück nach Origin
    pBoard[NUM_ROWS * oldTarget.yCoord + oldTarget.xCoord] = NULL; // auf Target steht dann nichts mehr
}

/* Berechnet das "Gewicht" eines Turms an einer gegebenen Koordinate. Dieses "Gewicht" soll ein Maß für die
 * Zusammensetzung des Turms aus eigenen oder gegnerischen Steinen sein, damit abgeschätzt werden kann, ob es sich lohnt,
 * diesen Turm anzugreifen. Viele eigene Steine im Turm -> hohes Gewicht, viele gegnerische Steine -> niedriges Gewicht.
 * Damen zählen mehrfach. */
float computeTowerWeight(coordinate square) {
    float weight = 0;
    tower *pCurrent = getPointerToSquare(square);

    while (pCurrent != NULL) { // Schleife über alle Steine im Turm
        // betrachte den Stein, auf den gerade pCurrent zeigt, und addiere/subtrahiere zu weight
        if (pCurrent->piece == ownNormalTower) { weight += NORMAL_PIECE_WEIGHT; }
        else if (pCurrent->piece == ownQueenTower) { weight += QUEEN_TO_NORMAL_RATIO * NORMAL_PIECE_WEIGHT; }
        else if (pCurrent->piece == opponentNormalTower) { weight -= NORMAL_PIECE_WEIGHT; }
        else if (pCurrent->piece == opponentQueenTower) { weight -= QUEEN_TO_NORMAL_RATIO * NORMAL_PIECE_WEIGHT; }
        pCurrent = pCurrent->next;
    }
    return weight;
}

/* Nimmt die Ursprungs- und Zielkoordinaten einer (gültigen!) eigenen Turmverschiebung entgegen und gibt ein Rating
 * zurück, wie gut dieser Zug ist. Rating wird nach folgenden Kriterien bestimmt:
 *
 * - Es gibt ein Grundrating, das jeder gültige Zug einfach hat.
 * - Ein Zug mit einem normalen Turm ist ein kleines bisschen besser als ein Zug mit einer Dame, weil es einen Schritt
 *   Richtung Damenumwandlung bedeutet.
 * - Zusätzlich wird betrachtet, wie der Zug die Gefahrenlage ändert: Wenn danach eigene Spielsteine bedroht werden,
 *   ist das schlecht -> bestrafen. Wenn danach gegnerische Spielsteine bedroht werden, ist das gut -> belohnen (aber
 *   nicht um den gleichen Faktor wie eigene Bedrohung, da der Gegner ja als nächstes dran ist und reagieren kann). */
float evaluateMove(coordinate origin, coordinate target) {
    float rating = POSSIBLE_MOVE_BASE_RATING;

    if (getTopPiece(origin) == ownNormalTower) {
        rating += ADVANCE_NORMAL_PIECE_BONUS;
    }

    int piecesInDangerOwnPrev = howManyInDangerOwn();
    int piecesInDangerOppPrev = howManyInDangerOpponent();

    moveTower(origin, target);

    int piecesInDangerOwnAfter = howManyInDangerOwn();
    int piecesInDangerOppAfter = howManyInDangerOpponent();

    int dangerDifferenceOwn = piecesInDangerOwnAfter - piecesInDangerOwnPrev;
    if (dangerDifferenceOwn > 0) {
        printf(" * Mit diesem Zug bringe ich einen meiner Spielsteine in Gefahr...\n");
    } else if (dangerDifferenceOwn < 0) {
        printf(" * Mit diesem Zug bringe ich einen eigenen gefährdeten Spielstein in Sicherheit.\n");
    }
    rating -= OWN_ENDANGERMENT_FACTOR * dangerDifferenceOwn;

    int dangerDifferenceOpp = piecesInDangerOppAfter - piecesInDangerOppPrev;
    if (dangerDifferenceOpp > 0) {
        printf(" * Mit diesem Zug bedrohe ich einen gegnerischen Spielstein.\n");
        rating += THREATEN_OPPONENT_FACTOR * dangerDifferenceOpp;
    }

    if (justConvertedToQueen) {
        printf(" * Mit diesem Zug kann ich eine Damenumwandlung durchführen!\n");
        rating += CONVERT_TO_QUEEN_BONUS;
    }

    undoMoveTower(origin, target);

    return rating;
}

/* Gegeben eine Koordinate, an der ein Turm steht. Testet, an welche Felder der Turm verschoben werden kann. */
move tryAllMoves(coordinate origin) {
    char strOrigin[COORDINATE_LENGTH + 1];
    char strTarget[COORDINATE_LENGTH + 1];
    int newX, newY;
    coordToCode(strOrigin, origin);

    move bestMove = createMoveStruct();
    float rating;

    char originTopPiece = getTopPiece(origin);

    // normaler Turm
    if (originTopPiece == 'w' || originTopPiece == 'b') {

        if (originTopPiece == 'w') { newY = origin.yCoord + 1; }
        else { newY = origin.yCoord - 1; }

        // überprüfe Bewegung nach vorne links und nach vorne rechts
        for (int round = 0; round < POSSIBLE_DIRECTIONS_FORWARD; round++) {
            switch (round) {
                case 0:
                    newX = origin.xCoord + 1;
                    break;
                case 1:
                    newX = origin.xCoord - 1;
                    break;
            }

            coordinate possibleTarget = numsToCoord(newX, newY);

            if (checkMove(origin, possibleTarget) == 0) {
                coordToCode(strTarget, possibleTarget);
                printf("Von %s nach %s Bewegen ist ein gülter Zug.\n", strOrigin, strTarget);

                rating = evaluateMove(origin, possibleTarget);
                if (rating > bestMove.rating) {
                    bestMove.origin = origin;
                    bestMove.target = possibleTarget;
                    bestMove.rating = rating;
                }
            }
        }
    }

    // Turm mit Dame oben
    if (originTopPiece == 'W' || originTopPiece == 'B') {

        // überprüfe alle vier Richtungen
        int xDirection, yDirection;
        for (int round = 0; round < POSSIBLE_DIRECTIONS_FORWARD_AND_BACKWARD; round++) {
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

            // versuche für Target, 1-7 Felder in die jeweilige Richtung zu gehen
            for (int i = 1; i < NUM_ROWS; i++) {
                newX = origin.xCoord + i * xDirection;
                newY = origin.yCoord + i * yDirection;

                coordinate possibleTarget = numsToCoord(newX, newY);

                if (checkMove(origin, possibleTarget) != 0) {
                    break; // wenn Bewegen um i in diese Richtung ungültig ist, können wir abbrechen, denn Bewegen um i+1 ist dann sicher auch ungültig
                }

                coordToCode(strTarget, possibleTarget);
                printf("Von %s nach %s Bewegen ist ein gülter Zug.\n", strOrigin, strTarget);

                rating = evaluateMove(origin, possibleTarget);
                if (rating > bestMove.rating) {
                    bestMove.origin = origin;
                    bestMove.target = possibleTarget;
                    bestMove.rating = rating;
                }
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
 *   um den gleichen Faktor wie eigene Bedrohung, da der Gegner ja als nächstes dran ist und reagieren kann)
 *
 *   verbose deaktiviert die Kommandozeilenausgabe der Bewertungsstrings. Das ist deshalb gedacht, weil innerhalb dieser
 *   Funktion tryCaptureAgain und darin wiederum evaluateCapture aufgerufen wird, um zu überprüfen, ob sich nach diesem
 *   Teilzug weiterziehen lässt. Für diese Funktionsaufrufe sollte aber verbose abgestellt werden, da sonst in der
 *   Ausgabe Bewertungsstrings zu den aktuell zu vergleichenden Teilzügen mit Bewertungsstrings zu potentiellen
 *   Folgezügen vermischt werden. */
float evaluateCapture(coordinate origin, coordinate target, bool verbose) {
    float rating = POSSIBLE_MOVE_BASE_RATING;

    int ownWeight = computeTowerWeight(origin);
    if (verbose && ownWeight < 0) { printf(" * Der Turm, mit dem ich schlagen will, ist ziemlich schwach. Damit anzugreifen ist vielleicht nicht so gut.\n"); }
    rating += WEIGHT_RATING_FACTOR * ownWeight;

    coordinate victim = computeVictim(origin, target);
    int victimWeight = computeTowerWeight(victim);
    if (verbose && victimWeight < -1) { printf(" * Der gegnerische Turm ist ziemlich stark. Vielleicht sollte ich lieber einen anderen angreifen.\n"); }
    rating += WEIGHT_RATING_FACTOR * victimWeight;

    if (getTopPiece(victim) == opponentQueenTower) {
        if (verbose) { printf(" * Mit diesem Zug schlage ich eine gegnerische Dame!\n"); }
        rating += CAPTURE_QUEEN_BONUS;
    }

    int piecesInDangerOwnPrev = howManyInDangerOwn();
    int piecesInDangerOppPrev = howManyInDangerOpponent();

    captureTower(origin, target);

    move potentialNextMove = tryCaptureAgain(origin, target, false);
    if (potentialNextMove.origin.xCoord != potentialNextMove.target.xCoord ||
        potentialNextMove.origin.yCoord != potentialNextMove.target.yCoord) {
        if (verbose) { printf(" * Dieser Schlag ist gut, weil ich danach noch weiterschlagen kann.\n"); }
        rating += ANOTHER_CAPTURE_POSSIBLE_BONUS;
    } else {
        int piecesInDangerOwnAfter = howManyInDangerOwn();
        int dangerDifferenceOwn = piecesInDangerOwnAfter - piecesInDangerOwnPrev;
        if (verbose) {
            if (dangerDifferenceOwn > 0) {
                printf(" * Mit diesem Zug bringe ich einen meiner Spielsteine in Gefahr...\n");
            } else if (dangerDifferenceOwn < 0) {
                printf(" * Mit diesem Zug bringe ich einen eigenen gefährdeten Spielstein in Sicherheit.\n");
            }
        }
        rating -= OWN_ENDANGERMENT_FACTOR * dangerDifferenceOwn;
    }

    int piecesInDangerOppAfter = howManyInDangerOpponent();
    int dangerDifferenceOpp = piecesInDangerOppAfter - piecesInDangerOppPrev;
    if (dangerDifferenceOpp > 0) {
        printf(" * Mit diesem Zug bedrohe ich einen gegnerischen Spielstein.\n");
        rating += THREATEN_OPPONENT_FACTOR * dangerDifferenceOpp;
    }

    if (justConvertedToQueen) {
        if (verbose) printf(" * Mit diesem Schlag kann ich eine Damenumwandlung durchführen!\n");
        rating += CONVERT_TO_QUEEN_BONUS;
    }

    undoCaptureTower(origin, target);

    return rating;
}


/* Gegeben eine Koordinate, an der ein Turm steht, und eine zweite "blockierte" Koordinate. Testet alle Schläge
 * die der Turm an der ersten Koordinate ausführen könnte – außer solche, die in die Richtung des blockierten Felds
 * führen würden. (tryAllCaptures ohne blockiertes Feld lässt sich dann als Spezialfall von diesem sehen)
 *
 * Gibt den besten möglichen Zug zurück. Wenn keine Schläge möglich sind: Gibt Standard-move mit
 * origin- und target-Koordinaten -1 und rating -FLT_MAX zurück.
 *
 * verbose deaktiviert die Kommandozeilenausgabe der Bewertungsstrings. Siehe Kommentar zu evaluateCapture, warum
 * das wünschenswert ist.*/
move tryAllCapturesExcept(coordinate origin, coordinate blocked, bool verbose) {
    char strOrigin[COORDINATE_LENGTH + 1];
    char strTarget[COORDINATE_LENGTH + 1];
    int newX, newY;
    coordToCode(strOrigin, origin);

    move bestMove = createMoveStruct();
    float rating;

    char originTopPiece = getTopPiece(origin);

    // normaler Turm
    if (originTopPiece == 'w' || originTopPiece == 'b') {
        // überprüfe alle vier Richtungen bzw. alle vier potentiellen Targetfelder
        for (int round = 0; round < POSSIBLE_DIRECTIONS_FORWARD_AND_BACKWARD; round++) {
            switch (round) {
                case 0:
                    newX = origin.xCoord + DISTANCE_FOR_VALID_CAPTURE;
                    newY = origin.yCoord + DISTANCE_FOR_VALID_CAPTURE;
                    break;
                case 1:
                    newX = origin.xCoord + DISTANCE_FOR_VALID_CAPTURE;
                    newY = origin.yCoord - DISTANCE_FOR_VALID_CAPTURE;
                    break;
                case 2:
                    newX = origin.xCoord - DISTANCE_FOR_VALID_CAPTURE;
                    newY = origin.yCoord + DISTANCE_FOR_VALID_CAPTURE;
                    break;
                case 3:
                    newX = origin.xCoord - DISTANCE_FOR_VALID_CAPTURE;
                    newY = origin.yCoord - DISTANCE_FOR_VALID_CAPTURE;
                    break;
            }

            if (newX == blocked.xCoord && newY == blocked.yCoord) {
                continue; // wenn mich newX/newY genau auf das blockierte Feld führen -> diese Iteration überspringen
            }

            coordinate possibleTarget = numsToCoord(newX, newY);

            if (checkCapture(origin, possibleTarget) == 0) {
                if (verbose) {
                    coordToCode(strTarget, possibleTarget);
                    printf("Von %s nach %s Schlagen ist ein gülter Zug.\n", strOrigin, strTarget);
                }

                rating = evaluateCapture(origin, possibleTarget, verbose);
                if (rating > bestMove.rating) {
                    bestMove.origin = origin;
                    bestMove.target = possibleTarget;
                    bestMove.rating = rating;
                }
            }
        }
    }

    // Turm mit Dame oben
    if (originTopPiece == 'W' || originTopPiece == 'B') {
        int xDirection, yDirection;
        // überprüfe alle vier Richtungen: eine Iteration pro Richtung
        for (int round = 0; round < POSSIBLE_DIRECTIONS_FORWARD_AND_BACKWARD; round++) {
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
                if (xDirection == blockedXDirection && yDirection == blockedYDirection) {
                    continue; // diese Iteration (d.h. diese Richtung) überspringen
                }
            }

            // für diese Iteration/Richtung: teste, ob 2, 3, ..., 7 Felder in diese Richtung schlagen möglich ist
            for (int i = DISTANCE_FOR_VALID_CAPTURE; i < NUM_ROWS; i++) {
                newX = origin.xCoord + i * xDirection;
                newY = origin.yCoord + i * yDirection;

                coordinate possibleTarget = numsToCoord(newX, newY);

                if (checkCapture(origin, possibleTarget) != 0) {
                    continue; // wenn dieser Schlag ungültig -> nächste Iteration, d.h. überprüfe Feld eins dahinter
                } else {
                    if (verbose) {
                        coordToCode(strTarget, possibleTarget);
                        printf("Von %s nach %s Schlagen ist ein gülter Zug.\n", strOrigin, strTarget);
                    }

                    rating = evaluateCapture(origin, possibleTarget, verbose);
                    if (rating > bestMove.rating) {
                        bestMove.origin = origin;
                        bestMove.target = possibleTarget;
                        bestMove.rating = rating;
                    }

                    break; // wenn dieser Schlag gültig, gibt es in dieser Richtung keine weiteren gültigen Züge
                }

            }
        }
    }

    return bestMove;
}

/* Gegeben eine Koordinate, an der ein Turm steht. Testet alle Schläge, die der Turm an dieser Koordinate ausführen
 * könnte, und gibt den besten davon zurück. [Spezialfall von tryAllCapturesExcept, das Except wird nicht genutzt] */
move tryAllCaptures(coordinate origin) {
    return tryAllCapturesExcept(origin, numsToCoord(-1, -1), true);
}

/* Um nach einem Teilzug weitere Folgezüge zu finden. Wendet tryAllCapturesExcept an. Wenn kein gültiger Folgezug mehr
 * existiert, werden in move origin und target beide auf die Ursprungskoordinate gesetzt.
 *
 * verbose deaktiviert die Kommandozeilenausgabe der Bewertungsstrings. Siehe Kommentar zu evaluateCapture, warum
 * das wünschenswert ist. */
move tryCaptureAgain(coordinate nowBlocked, coordinate newOrigin, bool verbose) {
    move ret = tryAllCapturesExcept(newOrigin, nowBlocked, verbose);

    // wenn tryAllCaptures keinen gültigen Zug finden kann: kein Zug mehr, kodiert indem origin = target
    if (ret.origin.xCoord == -1 || ret.origin.yCoord == -1 || ret.target.xCoord == -1 || ret.target.yCoord == -1) {
        ret.origin = newOrigin;
        ret.target = newOrigin;
    }

    return ret;
}

/* Nimmt einen String und dessen Länge als Parameter. Bestimmt den günstigen Spielzug auf Basis des aktuellen
 * Spielbretts. Schreibt diesen in den angegebenen String, sofern darin genug Platz, und gibt 0 zurück; sonst -1. */
int think(char *answer, int answerMaxLength) {
    move overallBestMove = createMoveStruct();
    move thisMove;
    int answerCounter = 0; // für Position im Antwortstring

    justConvertedToQueen = false;

    // versuche erst, einen gültigen Schlag-Teilzug zu finden
    for (int i = 0; i < NUM_ROWS; i++) {
        for (int j = 0; j < NUM_ROWS; j++) {
            char thisSquareTopPiece = getTopPiece(numsToCoord(i, j));
            if (thisSquareTopPiece != ownNormalTower && thisSquareTopPiece != ownQueenTower) {
                continue; // Felder, auf denen kein eigener Turm steht, überspringen
            }
            thisMove = tryAllCaptures(numsToCoord(i, j)); // berechnet den besten Spielzug, wenn von diesem Feld aus geschlagen wird
            if (thisMove.rating > overallBestMove.rating) {
                overallBestMove.origin = thisMove.origin;
                overallBestMove.target = thisMove.target;
                overallBestMove.rating = thisMove.rating;
            }
        }
    }

    // wenn ein gültiger Schlag-Teilzug gefunden werden kann: in answer schreiben, danach in Schleife nach weiteren Teilzügen vom Zielfeld aus suchen
    if (overallBestMove.origin.xCoord != -1 && overallBestMove.origin.yCoord != -1 &&
        overallBestMove.target.xCoord != -1 && overallBestMove.target.yCoord != -1) {
        coordToCode(answer, overallBestMove.origin);
        if (answerMaxLength <= answerCounter + COORDINATE_LENGTH + 1 + COORDINATE_LENGTH) { // teste, ob überhaupt Platz für 5 Zeichen (+\n) ist
            fprintf(stderr, "Fehler! Habe einen Schlag-Zug gefunden, aber im Antwortstring von think() ist nicht genug Platz für 5 Zeichen.\n");
            return -1;
        }
        answer[answerCounter + COORDINATE_LENGTH] = ':';
        coordToCode(answer + COORDINATE_LENGTH + 1, overallBestMove.target);
        answerCounter += (COORDINATE_LENGTH + 1 + COORDINATE_LENGTH);

        bool moreCapturesLoop = true;

        while (moreCapturesLoop) {
            printf("\nVerwende den Teilzug %s und suche weiter:\n", answer + (answerCounter - 5));
            // führe den besten Zug tatsächlich aus
            captureTower(overallBestMove.origin, overallBestMove.target);
            justConvertedToQueen = false;

            // wendet tryCaptureAgain() mit den Koordinaten des alten besten Zugs an: origin ist jetzt blockiert,
            // von target aus wird ein weiterer Teilzug gesucht; Ergebnis zurück in overallBestMove gespeichert
            overallBestMove = tryCaptureAgain(overallBestMove.origin, overallBestMove.target, true);

            if (overallBestMove.origin.xCoord == overallBestMove.target.xCoord &&
                overallBestMove.origin.yCoord == overallBestMove.target.yCoord) {
                // kein weiteres Schlagen mehr möglich
                printf("Von hier ist kein weiterer Teilzug möglich.\n");
                moreCapturesLoop = false;
            } else {
                // weiterer Schlagzug gefunden -> in answer schreiben
                if (answerMaxLength <= answerCounter + 1 + COORDINATE_LENGTH) { // teste, ob überhaupt Platz für nochmal 3 Zeichen (+\n) ist
                    fprintf(stderr, "Fehler! Im Antwortstring von think() ist nicht genug Platz für weitere 3 Zeichen eines Folgezugs.\n");
                    return -1;
                }
                answer[answerCounter] = ':';
                coordToCode(answer + answerCounter + 1, overallBestMove.target);
                answerCounter += (1 + COORDINATE_LENGTH);
            }

        }
    } else { // kein Schlag-Teilzug gefunden -> suche Beweg-Zug
        for (int i = 0; i < NUM_ROWS; i++) {
            for (int j = 0; j < NUM_ROWS; j++) {
                char thisSquareTopPiece = getTopPiece(numsToCoord(i, j));
                if (thisSquareTopPiece != ownNormalTower && thisSquareTopPiece != ownQueenTower) {
                    continue; // Felder, auf denen kein eigener Turm steht, überspringen
                }
                thisMove = tryAllMoves(numsToCoord(i, j));
                if (thisMove.rating > overallBestMove.rating) {
                    overallBestMove.origin = thisMove.origin;
                    overallBestMove.target = thisMove.target;
                    overallBestMove.rating = thisMove.rating;
                }
            }
        }

        if (answerMaxLength <= answerCounter + COORDINATE_LENGTH + 1 + COORDINATE_LENGTH) { // teste, ob überhaupt Platz für 5 Zeichen (+\n) ist
            fprintf(stderr, "Fehler! Habe einen Beweg-Zug gefunden, aber im Antwortstring von think() ist nicht genug Platz für 5 Zeichen.\n");
            return -1;
        }
        coordToCode(answer, overallBestMove.origin);
        answer[answerCounter + COORDINATE_LENGTH] = ':';
        coordToCode(answer + COORDINATE_LENGTH + 1, overallBestMove.target);
        answerCounter += (COORDINATE_LENGTH + 1 + COORDINATE_LENGTH);
    }

    answer[answerCounter] = '\n'; // answer braucht am Ende ein Newline, sonst wird die Nachricht in mainloop_filehandler liegengelassen
    return 0;
}

// Räumt auf: gemallocten Speicher freigeben, Shmemory-Segmente löschen.
int cleanupThinkerfunctions(void) {
    int ret = 0;

    if (shmidMoves != -1) {
        if (shmDelete(shmidMoves) > 0) { ret = -1; }
    }

    if (pBoard != NULL) {
        free(pBoard);
        pBoard = NULL;
    }

    if (pPieces != NULL) {
        free(pPieces);
        pPieces = NULL;
    }

    return ret;
}
