#ifndef _EVAL_H_
#define _EVAL_H_

#include "common.h"
#include "board.h"
#include "bitboard.h"
#include "math.h"

#define MG 0
#define EG 1

#define DISTANCE(s1,s2) (abs((s1>>3) - (s2>>3)) + abs((s1&7)-(s2&7)))

#define EGTHRESH 2580
#define MGTHRESH 5480

int evaluate(board_t*);

bool is_draw(board_t*,search_info_t*);

#endif