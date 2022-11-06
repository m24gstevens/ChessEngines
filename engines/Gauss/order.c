#include "order.h"

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

static inline void score_move(board_t* board, search_info_t* si, move_t* move) {
    int score = 0;
    U16 mov = move->move;
    U8 flags = MOVE_FLAGS(mov);
    if (flags & 0x4) {
        score += 10000 + MVV_LVA[board->squares[MOVE_TO(mov)]][board->squares[MOVE_FROM(mov)]];
    } else {
        if (mov == si->killers[si->ply][0]) {
            score += 9000;
        } else if (mov == si->killers[si->ply][1]) {
            score += 8000;
        }
    }
    if (flags & 0x8) {
        if (flags & 0x3 == 0x3) {
            score += 5000;
        }
    }

    move->score = score;
}

void score_moves(board_t* board, search_info_t* si, move_t* ms, int nm) {
    int i;
    for (i=0;i<nm;i++) {
        score_move(board,si,(ms+i));
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