#ifndef _SEE_H_
#define _SEE_H_

#include "common.h"
#include "bitboard.h"
#include "moves.h"

extern const int simple_piece_values[13];

int SEE(board_t*, enumSide, U16);

bool good_capture(board_t*, U16);

#endif