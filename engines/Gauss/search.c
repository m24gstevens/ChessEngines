#include "search.h"
#include "eval.h"
#include "order.h"
#include "uci.h"
#include "tt.h"

static inline void update_pv(search_info_t* si, U16 move, int ply) {
    int i;
    si->pv[ply][ply] = move;
    for (i=ply+1;si->pv[ply+1][i]!=NOMOVE;i++) {si->pv[ply][i] = si->pv[ply+1][i];}
    si->pv[ply][i] = NOMOVE;
}

static inline void info_pv(board_t* board, search_info_t* si) {
    int d;
    U16 best;
    board_t bc = *board;
    hist_t undo;

    printf("info pv ");
    for (d=0;d<si->sdepth;d++) {
        best = NOMOVE;
        probe_tt(&bc,d,0,0,0,&best);

        if (best == NOMOVE) {break;}
        print_move(best);
        printf(" ");

        make_move(&bc,best,&undo);
    }
    printf("\n");
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
    score_moves(board,si,mp,nmov,NOMOVE);

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

int search(board_t* board, int depth, int alpha, int beta, move_t* msp, search_info_t* si, bool is_pv) {
    int score,nmov,ct,i,in_check,legal,ply;
    move_t* mp = msp;
    U16 move, pvmove=NOMOVE,bestmove;
    U8 ttflag = TT_ALPHA;
    bool next_pv;

    if (!(si->nodes & 2047)) { communicate(si);}
    if (time_control.stop) {return 0;}

    ply=si->ply;
    if (ply > MAXPLY - 1) {
        return evaluate(board);
    }

    if (depth < 1) {
        return quiesce(board, alpha, beta, msp, si);
    }

    if ((score = probe_tt(board,ply,depth,alpha,beta,&pvmove)) != INVALID) {
        if (!is_pv || (score > alpha && score < beta)) {
            return score;
        }
    }

    si->nodes++;

    enumSide side = board->side;
    in_check = is_square_attacked(board,bitscanForward(board->bitboards[K+6*side]),1^side);

    hist_t undo;
    nmov = generate_moves(board, msp);
    si->msp[ply+1] = mp+nmov;
    score_moves(board,si,mp,nmov,pvmove);
    
    legal=0;

    while ((move = pick_move(mp++,nmov--)) != NOMOVE) {
        if (!make_move(board,move,&undo)) {continue;}
        si->ply++;
        next_pv = (move == pvmove) & is_pv;
        score = -search(board, depth-1, -beta, -alpha, mp+nmov, si,next_pv);
        legal++;
        unmake_move(board,move,&undo);
        si->ply--;

        if (score > alpha) {
            alpha=score;
            bestmove = move;
            ttflag = TT_EXACT;
            update_pv(si,move,ply);
        }
        if (score >= beta) {
            if (!MOVE_FLAGS(move) & 0x4) {
                si->killers[1][ply] = si->killers[0][ply];
                si->killers[0][ply] = move;
            }
            ttflag = TT_BETA;
            alpha = beta;
            break;
        }
    }

    if (legal==0) {
        return in_check ? -MATE_SCORE+ply : 0;
    }

    store_tt(board,depth,ply,alpha,ttflag,bestmove);

    return alpha;
}

int search_root(board_t* board, int depth, int alpha, int beta, search_info_t* si) {
    int score,nmov,ct,i,in_check,legal,ply;
    move_t movestack[MAXMOVES], *mp = movestack;
    si->msp[0] = movestack;
    si->sdepth = depth;

    U16 move, pvmove=NOMOVE;
    bool is_pv;

    si->nodes++;

    enumSide side = board->side;
    in_check = is_square_attacked(board,bitscanForward(board->bitboards[K+6*side]),1^side);

    hist_t undo;
    nmov = generate_moves(board, movestack);
    si->msp[1] = mp+nmov;
    score_moves(board,si,mp,nmov,si->bestmove);
    
    legal=0;

    while ((move = pick_move(mp++,nmov--)) != NOMOVE) {
        if (!make_move(board,move,&undo)) {continue;}
        si->ply++;
        is_pv = (move == si->bestmove);
        score = -search(board, depth-1, -beta, -alpha, mp+nmov, si,is_pv);
        legal++;
        unmake_move(board,move,&undo);
        si->ply--;

        if (score > alpha) {
            si->bestmove = move;
            update_pv(si,move,ply);

            if (score >= beta) {
                if (!MOVE_FLAGS(move) & 0x4) {
                    si->killers[1][ply] = si->killers[0][ply];
                    si->killers[0][ply] = move;
                }
                store_tt(board,depth,0,beta,TT_BETA,si->bestmove);
                info_pv(board,si);
                return beta;
            }
            alpha=score;
        }
    }

    store_tt(board,depth,0,alpha,TT_EXACT,si->bestmove);
    printf("info depth %d cp %d nodes %ld\n",depth,score,si->nodes);
    info_pv(board,si);

    return alpha;
}

int search_widen(board_t* board, int depth, int score, search_info_t* si) {
    int alpha,beta,val;
    alpha = score - 100;
    beta = score + 100;
    val = search_root(board,depth,alpha,beta,si);
    if (val <= alpha || val >= beta) {
        val = search_root(board, depth, -INF, INF, si);
    }
    return val;
}

void iterative_deepening(board_t* board) {
    int score, depth;
    U16 best;

    search_info_t si;
    si.ply = 0;
    si.nodes = 0L;
    si.bestmove = NOMOVE;

    depth = 1;
    score = search_root(board, 1, -INF, INF, &si);

    for (depth=2;depth<=MAXPLY;depth++) {
        if (time_control.stop || time_stop_root(&si)) {break;}
        best = si.bestmove;

        score = search_widen(board,depth,score,&si);
    }
    send_move(best);
}

void search_position(board_t* board) {
    time_calc();
    iterative_deepening(board);
}