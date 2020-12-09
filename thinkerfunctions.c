#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "thinkerfunctions.h"

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

coordinate numsToCoord(int x, int y) {
    coordinate ret;
    ret.xCoord = x;
    ret.yCoord = y;
    return ret;
}

tower getSquare(tower *board[64], coordinate c) {

    if (c.xCoord == -1 || c.yCoord == -1) {
        fprintf(stderr, "Fehler! Ungültige Koordinate.\n");
        tower t;
        t.piece = 'X';
        return t;
    } else {
        return *board[8*c.yCoord + c.xCoord];
    }

}

char getTopPiece(tower *board[64], coordinate c) {
    return getSquare(board, c).piece;
}

 void printTopPieces(tower *board[64]) {
    for (int i = 7; i >= 0; i--) {
        for (int j = 0; j < 8; j++) {
            printf("%c ", getTopPiece(board, numsToCoord(j, i)));
        }
        printf("\n");
    }
}

void addToSquare(tower *board[64], coordinate c, char piece) {

    if (c.xCoord == -1 || c.yCoord == -1) {
        fprintf(stderr, "Fehler! Ungültige Koordinaten.\n");
    } else {
        if (board[8*c.yCoord + c.xCoord]->piece == '0' && board[8*c.yCoord + c.xCoord]->next == NULL) {
            board[8*c.yCoord + c.xCoord]->piece = piece;
        } else {
            tower *newTower;
            newTower = (tower *) malloc(sizeof(tower));

            newTower->piece = piece;
            newTower->next = board[8*c.yCoord + c.xCoord];
            board[8*c.yCoord + c.xCoord] = newTower;
        }
    }
}



// char* coordToCode (coordinate c) {
//    char ret[2];
//
//    switch(c.xCoord) {
//        case 0:
//            ret[0] = 'A';
//            break;
//        case 1:
//            ret[0] = 'B';
//            break;
//        case 2:
//            ret[0] = 'C';
//            break;
//        case 3:
//            ret[0] = 'D';
//            break;
//        case 4:
//            ret[0] = 'E';
//            break;
//        case 5:
//            ret[0] = 'F';
//            break;
//        case 6:
//            ret[0] = 'G';
//            break;
//        case 7:
//            ret[0] = 'H';
//            break;
//        default:
//            ret[0] = 'X';
//            break;
//    }
//    switch(c.yCoord) {
//        case 0:
//            ret[1] = '1';
//            break;
//        case 1:
//            ret[1] = '2';
//            break;
//        case 2:
//            ret[1] = '3';
//            break;
//        case 3:
//            ret[1] = '4';
//            break;
//        case 4:
//            ret[1] = '5';
//            break;
//        case 5:
//            ret[1] = '6';
//            break;
//        case 6:
//            ret[1] = '7';
//            break;
//        case 7:
//            ret[1] = '8';
//            break;
//        default:
//            ret[1] = 'X';
//            break;
//    }
//
//    return ret;
//}