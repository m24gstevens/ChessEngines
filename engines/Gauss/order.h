#ifndef _ORDER_H_
#define _ORDER_H_

#include "common.h"
#include "moves.h"

#define SCORE_PV 90000000
#define SCORE_WCAPTURE 82000000
#define SCORE_CAPTURE 81000000
#define SCORE_LCAPTURE 80000000
#define SCORE_PROMOTE 80000500
#define SCORE_KILLER1 80000400
#define SCORE_KILLER2 80000300
#define SCORE_COUNTER 80000200
#define SCORE_UNDERPROM 80000100
#define SCORE_HIST_MAX 80000000

extern int history_table[2][64][64];

void clear_history();
void age_history();
void half_history();

void score_moves_qsearch(board_t* board, search_info_t* si, move_t* ms, int nm, U16 pvmove);
void score_moves(board_t* board, search_info_t* si, move_t* ms, int nm, U16 pvmove);

U16 pick_move(move_t* ms, int nm);
U16 pick_move_qsearch(move_t* ms, int nm);

#endif