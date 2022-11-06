#ifndef _ORDER_H_
#define _ORDER_H_

#include "common.h"
#include "moves.h"

void score_moves(board_t* board, search_info_t* si, move_t* ms, int nm);

U16 pick_move(move_t* ms, int nm);

#endif