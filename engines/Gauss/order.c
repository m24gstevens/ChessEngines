#include "order.h"
#include "see.h"

// history_heuristic[side][from][to]
int history_heuristic[2][64][64];

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

void clear_history_heuristic() {
    memset(history_heuristic,0,sizeof(int)*2*64*64);
}

void age_history_heuristic() {
    for (int i=0; i<2; i++) {
        for (int j=0; j<64; j++) {
            for (int k=0; k<64; k++) {
                history_heuristic[i][j][k] >>= 4;
            }
        }
    }
}

void calibrate_history_heuristic() {
    for (int i=0; i<2; i++) {
        for (int j=0; j<64; j++) {
            for (int k=0; k<64; k++) {
                history_heuristic[i][j][k] >>= 1;
            }
        }
    }
}


void scores_cutoff(board_t* board, search_info_t* si, U16 move, int depth) {
    si->killers[1][si->ply] = si->killers[0][si->ply];
    si->killers[0][si->ply] = move;
    history_heuristic[board->side][MOVE_FROM(move)][MOVE_TO(move)] += depth;
    if (history_heuristic[board->side][MOVE_FROM(move)][MOVE_TO(move)] > SCORE_HIST_MAX) {calibrate_history_heuristic();}
}

void swap_moves(move_t* m1, move_t* m2) {
    m1->move ^= m2->move;
    m2->move ^= m1->move;
    m1->move ^= m2->move;

    m1->score ^= m2->score;
    m2->score ^= m1->score;
    m1->score ^= m2->score;
}

void score_move(board_t* board, search_info_t* si, move_t* mp, U16 pvmove) {
    int score = 0, seeval;

    if (mp->move == pvmove) {
        score = SCORE_PV;
        goto scoring;
    }

    if (IS_CAPTURE(mp->move)) {
        score = SCORE_CAPTURE + MVV_LVA[board->squares[MOVE_TO(mp->move)]][board->squares[MOVE_FROM(mp->move)]];

        if (good_capture(board, mp->move) == 1) {
            score += GOODCAP_BONUS;
        } 

    } else {
        if (!IS_PROMOTION(mp->move)) {
            if (mp->move == si->killers[0][si->ply]) {score = SCORE_KILLER1;}
            else if (mp->move == si->killers[1][si->ply]) {score = SCORE_KILLER2;}
            else {score = history_heuristic[board->side][board->squares[MOVE_FROM(mp->move)]][board->squares[MOVE_TO(mp->move)]];}
        } else {
            if (MOVE_PROMOTE_TO(mp->move) == 3) {
                score = SCORE_PROMOTE;
            } else {score = SCORE_UNDERPROMOTE;}

        }
    }

    scoring:
        mp->score = score;
}

void score_moves(board_t* board, search_info_t* si, move_t* mp, int nmoves, U16 pvmove) {
    int i;
    for (i=0; i<nmoves; i++) {
        score_move(board, si, (mp+i), pvmove);
    }
}


void print_moves(board_t* board, move_t* mp, int nmoves) {
    int i;
    for (i=0; i<nmoves; i++) {
        printf("Move ");
        print_move((mp+i)->move);
        printf(" Score %d\n", (mp+i)->score);
    }
}

void sort_moves(move_t* mp, int nmoves) {
    int c,n;
    for (c = 0;c<nmoves; c++) {
        for (n=c+1; n<nmoves; n++) {
            if ((mp+c)->score < (mp+n)->score) {
                swap_moves(mp+c,mp+n);
            }
        }
    }
}

U16 pick_move(move_t* mp, int nmoves) {
    int i, bestidx = -1, bestscore = -1;
    if (!nmoves) { return NOMOVE; }

    for (i=0; i<nmoves; i++) {
        if ((mp+i)->score > bestscore) {
            bestidx = i;
            bestscore = (mp+i)->score;
        }
    }
    if (bestidx != 0) {
        swap_moves(mp, mp+bestidx);
    }
    return mp->move;
}