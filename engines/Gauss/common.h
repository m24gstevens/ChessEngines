#ifndef _COMMON_H_
#define _COMMON_H_

#include "gauss.h"

// Positions

extern char *starting_position;
extern char *kiwipete;
extern char *capture_position;
extern char *unsafe_king;

// PRNG

U64 random_u64_sparse();

extern U64 piece_keys[13][64];
extern U64 ep_keys[2][8];
extern U64 castle_keys[16];
extern U64 side_key;

U64 hash_position(board_t*);

// Timing

long get_time_ms();

// Inits

void init_tables();

// History

extern U64 game_history[MAXHIST];
extern int hply;

#endif