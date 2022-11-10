#ifndef _TRANSPOSITION_H_
#define _TRANSPOSITION_H_

#include "common.h"

enum ttFlag {HASH_FLAG_ALPHA, HASH_FLAG_BETA, HASH_FLAG_EXACT};
#define INVALID 1000000

extern hash_t* tt;
extern hash_eval_t* eval_tt;

void tt_setsize(long);

int probe_tt(board_t*, int, int, int, int, U16*);
void store_tt(board_t*, int, int, int, U8, U16);
void age_tt();

int probe_eval_tt(U64);
void store_eval_tt(U64,int);
void clear_eval_tt();

#endif