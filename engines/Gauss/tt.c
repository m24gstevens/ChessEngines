#include "tt.h"

hash_t* tt = NULL;

long tt_size = 0;   // Num entries - 1

void tt_setsize(long size) {
    if (tt != NULL) {free(tt);}

    if (size < 16) {
        tt_size = 0;
        return;
    }

    tt_size = size/sizeof(hash_t);
    while (tt_size & (tt_size - 1)) {
        tt_size &= (tt_size - 1);
    }
    tt = (hash_t *) malloc(tt_size * sizeof(hash_t));
    tt_size -= 1;
}

int probe_tt(board_t* board, int ply, int depth, int alpha, int beta, U16* best) {
    int score;
    if (!tt_size) {return INVALID;}

    hash_t* phashe = &tt[board->hash & tt_size];

    if (phashe->hash == board->hash) {
        *best = phashe->bestmove;

        if (phashe->depth >= depth) {
            score = phashe->val;
            if (score > MATE_THRESHOLD) {score -= ply;}
            if (score < -MATE_THRESHOLD) {score += ply;}

            if (phashe->flags == TT_EXACT) {return score;}
            if ((phashe->flags == TT_ALPHA) && (phashe->val <= alpha)) {return alpha;}
            if ((phashe->flags == TT_BETA) && (phashe->val >= beta)) {return beta;}
        }
    }
    return INVALID;
}

void store_tt(board_t* board, int depth, int ply, int val, U8 flags, U16 best) {
    if (!tt_size) {return;}

    hash_t* phashe = &tt[board->hash & tt_size];

    if ((phashe->hash == board->hash) && (phashe->depth > ply)) {return;}

    if (val > MATE_THRESHOLD) {val += ply;}
    if (val < -MATE_THRESHOLD) {val -= ply;}

    phashe->hash = board->hash;
    phashe->val = val;
    phashe->flags = flags;
    phashe->depth = depth;
    phashe->bestmove = best;
}