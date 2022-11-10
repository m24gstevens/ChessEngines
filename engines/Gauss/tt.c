#include "tt.h"
#include "moves.h"

hash_t* tt = NULL;
hash_eval_t* eval_tt = NULL;

long tt_size = 0;   // Num entries - 1
long eval_tt_size = 0;

void clear_tt() {
    for (int index = 0; index <= tt_size; index++) {
        hash_t* entry = &tt[index];
        entry->hash = C64(0);
        entry->val = 0;
        entry->depth = 0;
        entry->bestmove = 0;
        entry->flags = 0;
        entry->valid = false;
    }
}

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

    eval_tt_size = tt_size / 2;
    tt = (hash_t *) malloc(tt_size * sizeof(hash_t));
    eval_tt = (hash_eval_t*) malloc(eval_tt_size * sizeof(hash_eval_t));
    tt_size -= 1;
    eval_tt_size -= 1;

    clear_eval_tt();
    clear_tt();
}

int probe_tt(board_t* board, int ply, int depth, int alpha, int beta, U16* best) {
    int score;
    if (!tt_size) {return INVALID;}

    hash_t* phashe = &tt[board->hash & tt_size];

    if (phashe->valid && phashe->hash == board->hash) {
        *best = phashe->bestmove;

        if (phashe->depth >= depth) {
            score = phashe->val;
            if (score > MATE_THRESHOLD) {score -= ply;}
            if (score < -MATE_THRESHOLD) {score += ply;}

            if (phashe->flags == HASH_FLAG_EXACT) {return score;}
            if ((phashe->flags == HASH_FLAG_ALPHA) && (phashe->val <= alpha)) {return alpha;}
            if ((phashe->flags == HASH_FLAG_BETA) && (phashe->val >= beta)) {return beta;}
        }
    }
    return INVALID;
}

void store_tt(board_t* board, int depth, int ply, int val, U8 flags, U16 best) {
    if (!tt_size) {return;}

    hash_t* phashe = &tt[board->hash & tt_size];

    if ((phashe->hash == board->hash) && (phashe->depth > depth)) {return;}

    if (val > MATE_THRESHOLD) {val += ply;}
    if (val < -MATE_THRESHOLD) {val -= ply;}

    phashe->hash = board->hash;
    phashe->val = val;
    phashe->flags = flags;
    phashe->depth = depth;
    phashe->bestmove = best;
    phashe->valid = true;
}

void age_tt() {
    if (!tt_size) {return;}

    for (int i=0; i<=tt_size; i++) {
        tt[i].valid = false;
    }
}



int probe_eval_tt(U64 key) {
    int score;
    if (!eval_tt_size) {return INVALID;}

    hash_eval_t* phashe = &eval_tt[key & eval_tt_size];

    if ((phashe->valid == true) && (phashe->hash == key)) {
        return phashe->evaluation;
    }
    return INVALID;
}

void store_eval_tt(U64 key, int evaluation) {
    hash_eval_t* phashe = &eval_tt[key & eval_tt_size];

    phashe->hash = key;
    phashe->valid = true;
    phashe->evaluation = evaluation;
}

void clear_eval_tt() {
    if (!eval_tt_size) {return;}
    if (eval_tt == NULL) {return;}
    for (int i=0; i<=eval_tt_size;i++) {
        eval_tt[i].hash = C64(0);
        eval_tt[i].evaluation = INVALID;
        eval_tt[i].valid = false;
    }
}