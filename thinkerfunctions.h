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

coordinate codeToCoord(char code[2]);

coordinate numsToCoord(int x, int y);

tower getSquare(tower *board[64], coordinate c);

char getTopPiece(tower *board[64], coordinate c);

void addToSquare(tower *board[64], coordinate c, char piece);

void printTopPieces(tower *board[64]);

// char* coordToCode (coordinate c);

#endif //SYSPRAK_THINKERFUNCTIONS_H
