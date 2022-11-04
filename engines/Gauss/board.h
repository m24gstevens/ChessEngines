#ifndef _BOARD_H_ 
#define _BOARD_H_

#include "common.h"

typedef struct {
    U64 bitboards[12];
    U64 occupancies[3];
    U8 squares[64];
    short ep_square;
    U8 castle_flags;
    enumSide side;
    int rule50;
} board_t;




void print_board(board_t);

void parse_fen(char*, board_t*);

#endif