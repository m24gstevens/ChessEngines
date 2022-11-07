#ifndef _SEARCH_H_
#define _SEARCH_H_

#include "common.h"
#include "board.h"
#include "moves.h"

#define NULLMOVE_REDUCTION 2
#define DELTA_MARGIN 200

void search_position(board_t*);

#endif