#ifndef _ORDER_H_
#define _ORDER_H_

#include "common.h"
#include "moves.h"

#define PVSCORE 30000
#define CAPSCORE 10000
#define KILLER1 9000
#define KILLER2 8000
#define PROMSCORE 5000

void score_moves(board_t* board, search_info_t* si, move_t* ms, int nm, U16 pvmove);

U16 pick_move(move_t* ms, int nm);

#endif