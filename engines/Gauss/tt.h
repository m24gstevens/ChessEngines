#ifndef _TRANSPOSITION_H_
#define _TRANSPOSITION_H_

#include "common.h"

enum ttFlag {TT_ALPHA, TT_BETA, TT_EXACT};
#define INVALID 1000000

extern hash_t* tt;

void tt_setsize(long);

int probe_tt(board_t*, int, int, int, int, U16*);
void store_tt(board_t*, int, int, int, U8, U16);


#endif