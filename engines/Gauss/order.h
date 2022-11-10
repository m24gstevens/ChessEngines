#ifndef _ORDER_H_
#define _ORDER_H_

#include "common.h"
#include "moves.h"

#define SCORE_PV 9000000
#define SCORE_WCAPTURE 8200000
#define GOODCAP_BONUS 100000
#define SCORE_CAPTURE 8000000
#define SCORE_PROMOTE 8100000
#define SCORE_KILLER1 8004000
#define SCORE_KILLER2 8003000
#define SCORE_COUNTER 8002000
#define SCORE_UNDERPROMOTE 8000000
#define SCORE_HIST_MAX 8000000

extern int history_heuristic[2][64][64];
void clear_history_heuristic();
void age_history_heuristic();
void calibrate_history_heuristic();

void score_moves(board_t*, search_info_t*, move_t*, int, U16);

void print_moves(board_t*, move_t*, int);

void scores_cutoff(board_t*, search_info_t*, U16, int);

void sort_moves(move_t*, int);

U16 pick_move(move_t*, int);

#endif