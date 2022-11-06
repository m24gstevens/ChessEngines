#include "search.h"
#include "eval.h"
#include "order.h"

static inline void update_pv(search_info_t* si, U16 move, int ply) {
    int i;
    si->pv[ply][ply] = move;
    for (i=ply+1;si->pv[ply+1][i]!=NOMOVE;i++) {si->pv[ply][i] = si->pv[ply+1][i];}
    si->pv[ply][i] = NOMOVE;
}

static inline int quiesce(board_t* board, int alpha, int beta, move_t* msp, search_info_t* si) {
    int in_check, score, nmov, i, ct, stand_pat, ply;
    move_t* mp = msp;
    U16 move;

    si->nodes++;

    stand_pat = evaluate(board);
    if (stand_pat >= beta) {return beta;}
    if (stand_pat > alpha) {
        alpha=stand_pat;
    }

    ply=si->ply;

    hist_t undo;
    nmov = generate_captures(board, msp);
    si->msp[ply+1] = mp+nmov;
    score_moves(board,si,mp,nmov);

    while ((move = pick_move(mp++,nmov--)) != NOMOVE) {
        if (!make_move(board,move,&undo)) {continue;}
        si->ply++;
        score = -quiesce(board, -beta, -alpha, mp+nmov, si);
        unmake_move(board,move,&undo);
        si->ply--;

        if (score > alpha) {
            alpha=score;
            update_pv(si,move,ply);
        }
        if (score >= beta) {
            return beta;
        }
    }

    return alpha;
}

int negamax(board_t* board, int depth, int alpha, int beta, move_t* msp, search_info_t* si) {
    int score,nmov,ct,i,in_check,legal,ply;
    move_t* mp = msp;
    U16 move;

    ply=si->ply;

    if (depth==0) {
        return quiesce(board, alpha, beta, msp, si);
    }

    si->nodes++;

    enumSide side = board->side;
    in_check = is_square_attacked(board,bitscanForward(board->bitboards[K+6*side]),1^side);

    hist_t undo;
    nmov = generate_moves(board, msp);
    si->msp[ply+1] = mp+nmov;
    score_moves(board,si,mp,nmov);
    
    legal=0;

    while ((move = pick_move(mp++,nmov--)) != NOMOVE) {
        if (!make_move(board,move,&undo)) {continue;}
        si->ply++;
        score = -negamax(board, depth-1, -beta, -alpha, mp+nmov, si);
        legal++;
        unmake_move(board,move,&undo);
        si->ply--;

        if (score > alpha) {
            alpha=score;
            update_pv(si,move,ply);
        }
        if (score >= beta) {
            if (!MOVE_FLAGS(move) & 0x4) {
                si->killers[1][ply] = si->killers[0][ply];
                si->killers[0][ply] = move;
            }
            return beta;
        }
    }

    if (legal==0) {
        return in_check ? -49000+ply : 0;
    }

    return alpha;
}

void search_position(board_t* board, int depth) {
    move_t movestack[MAXMOVES];
    search_info_t si;
    si.ply = 0;
    si.nodes = 0L;
    si.msp[0] = movestack;
    int score = negamax(board,depth,-50000,50000,&movestack[0],&si);
    printf("bestmove ");
    print_move(si.pv[0][0]);
    printf("\n");
    printf("info depth %d nodes %ld\n",depth,si.nodes);
}