#ifndef _EVAL_H_
#define _EVAL_H_

#include "common.h"
#include "board.h"
#include "bitboard.h"

#define MG 0
#define EG 1

#define EGTHRESH 2580
#define MGTHRESH 5480

int evaluate(board_t*);

bool is_draw(board_t*,search_info_t*);

#endif