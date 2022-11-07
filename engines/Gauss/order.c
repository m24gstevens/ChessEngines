#include "order.h"
#include "see.h"

int history_table[2][64][64];

void clear_history() {
    memset(history_table,0,sizeof(int)*2*64*64);
}

void age_history() {
    for (int i=0; i<2; i++) {
        for (int j=0; j<64; j++) {
            for (int k=0; k<64; k++) {
                history_table[i][j][k] = history_table[i][j][k] / 8;
            }
        }
    }
}

void half_history() {
    for (int i=0; i<2; i++) {
        for (int j=0; j<64; j++) {
            for (int k=0; k<64; k++) {
                history_table[i][j][k] = history_table[i][j][k] / 2;
            }
        }
    }
}

const int MVV_LVA[13][12] = {
    60,65,64,63,62,61, 60,65,64,63,62,61,
    10,15,14,13,12,11, 10,15,14,13,12,11,
    20,25,24,23,22,21, 20,25,24,23,22,21,
    30,35,34,33,32,31, 30,35,34,33,32,31,
    40,45,44,43,42,41, 40,45,44,43,42,41,
    50,55,54,53,52,51, 50,55,54,53,52,51,

    60,65,64,63,62,61, 60,65,64,63,62,61,
    10,15,14,13,12,11, 10,15,14,13,12,11,
    20,25,24,23,22,21, 20,25,24,23,22,21,
    30,35,34,33,32,31, 30,35,34,33,32,31,
    40,45,44,43,42,41, 40,45,44,43,42,41,
    50,55,54,53,52,51, 50,55,54,53,52,51,

    10,15,14,13,12,11, 10,15,14,13,12,11,
};

void swap_moves(move_t* m1, move_t* m2) {
    m1->move ^= m2->move;
    m2->move ^= m1->move;
    m1->move ^= m2->move;

    m1->score ^= m2->score;
    m2->score ^= m1->score;
    m1->score ^= m2->score;
}

static inline void score_qsearch(board_t* board, search_info_t* si, move_t* move, U16 pvmove) {
    int score = 0, see;
    U16 mov = move->move;
    
    if (mov == pvmove) {score = SCORE_PV;}
    else {
        see = SEE(board,board->side,mov);
        if (see > 0) {score = SCORE_WCAPTURE;}
        else if (see == 0) {score = SCORE_CAPTURE;}
        else {score = SCORE_LCAPTURE;}
        score += MVV_LVA[board->squares[MOVE_TO(mov)]][board->squares[MOVE_FROM(mov)]];
    }

    move->score = score;
}

static inline void score_move(board_t* board, search_info_t* si, move_t* move, U16 pvmove) {
    int score = 0, see;
    U16 mov = move->move;
    U8 flags = MOVE_FLAGS(mov);

    if (mov == pvmove) {
        score = SCORE_PV;
        goto scoring;
    }

    if (flags & 0x4) {
        score = SCORE_WCAPTURE + MVV_LVA[board->squares[MOVE_TO(mov)]][board->squares[MOVE_FROM(mov)]];
    } else {
        if (mov == si->killers[si->ply][0]) {
            score = SCORE_KILLER1;
        } else if (mov == si->killers[si->ply][1]) {
            score = SCORE_KILLER2;
        } else if (board->last_move.piece != _) {
            if (((int)si->counter_moves[board->last_move.piece][board->last_move.to].piece == board->squares[MOVE_FROM(mov)])
                && ((int)si->counter_moves[board->last_move.piece][board->last_move.to].piece == MOVE_TO(mov))) {
                score = SCORE_COUNTER;
            }
        } else {
            score = history_table[board->side][MOVE_FROM(mov)][MOVE_TO(mov)];
        }
    }
    if (flags & 0x8) {
        if (flags & 0x3 == 0x3) {
            score = SCORE_PROMOTE;
        } else {
            score = SCORE_UNDERPROM;
        }
    }
    scoring:
        move->score = score;
}

void score_moves_qsearch(board_t* board, search_info_t* si, move_t* ms, int nm, U16 pvmove) {
    int i;
    for (i=0;i<nm;i++) {
        score_qsearch(board,si,(ms+i), pvmove);
    }
}

void score_moves(board_t* board, search_info_t* si, move_t* ms, int nm, U16 pvmove) {
    int i;
    for (i=0;i<nm;i++) {
        score_move(board,si,(ms+i), pvmove);
    }
}

U16 pick_move(move_t* ms, int nm) {
    if (!nm) {return NOMOVE;}

    int best_score = -1;
    int best_idx = -1;
    for (int i=0;i<nm;i++) {
        if ((ms+i)->score > best_score) {
            best_score = (ms+i)->score;
            best_idx = i;
        }
    }
    if (best_idx != 0) {swap_moves(ms,ms+best_idx);}
    return ms->move;
}

U16 pick_move_qsearch(move_t* ms, int nm) {
    if (!nm) {return NOMOVE;}

    int best_score = -1;
    int best_idx = -1;
    for (int i=0;i<nm;i++) {
        if ((ms+i)->score > best_score) {
            best_score = (ms+i)->score;
            best_idx = i;
        }
    }
    if (best_idx != 0) {swap_moves(ms,ms+best_idx);}
    if (ms->score < SCORE_CAPTURE) {return NOMOVE;}
    return ms->move;
}