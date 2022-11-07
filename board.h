#ifndef _BOARD_H_ 
#define _BOARD_H_

#include "common.h"

extern enumPiece char_to_piece_code[];


void print_board(board_t);

void parse_fen(board_t*,char*);

void setup_board(board_t*);

#endif