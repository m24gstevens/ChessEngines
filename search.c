#include "search.h"
#include "eval.h"
#include "order.h"
#include "uci.h"
#include "tt.h"
#include "see.h"

void prepare_search(search_info_t* si) {
    int i,j;
    for (i=0;i<12;i++) {
        for (j=0;j<64;j++) {
            si->counter_moves[i][j].piece = _;
            si->counter_moves[i][j].to = 0;
        }
    }
    si->follow_pv = true;
    memset(si->pv,0,sizeof(U16)*MAXPLY*MAXPLY);
}

static inline void update_pv(search_info_t* si, U16 move, int ply) {
    int i;
    si->pv[ply][ply] = move;
    for (i=ply+1;si->pv[ply+1][i]!=NOMOVE;i++) {si->pv[ply][i] = si->pv[ply+1][i];}
    si->pv[ply][i] = NOMOVE;
}

static inline void info_pv(board_t* board, search_info_t* si) {
    int d;
    U16 best;

    best = si->bestmove;
    d=0;
    while (((best = si->pv[0][d++]) != NOMOVE) && (d < si->sdepth)) {
        print_move(best);
        printf(" ");
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
    score_moves_qsearch(board,si,mp,nmov,NOMOVE);

    while ((move = pick_move(mp++,nmov--)) != NOMOVE) {
        if (!make_move(board,move,&undo)) {continue;}
        //if ((!IS_PROMOTION(move)) && (stand_pat + simple_piece_values[board->squares[MOVE_TO(move)]] + 200 < alpha)) {continue;}
        si->ply++;
        score = -quiesce(board, -beta, -alpha, mp+nmov, si);
        unmake_move(board,move,&undo);
        si->ply--;

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha=score;
            update_pv(si,move,ply);
        }
    }

    return alpha;
}

int search(board_t* board, int depth, int alpha, int beta, move_t* msp, search_info_t* si, bool is_pv) {
    int score,nmov,ct,i,in_check,tried,ply,new_depth,reduction,eval;
    move_t* mp = msp;
    hist_t undo;
    U16 move, pvmove=NOMOVE,bestmove;
    U8 ttflag = TT_ALPHA;
    bool next_pv, raised_alpha;

    if (!(si->nodes & 2047)) { communicate(si);}
    if (time_control.stop) {return 0;}

    if (alpha >= beta) {return alpha;}

    if (is_draw(board,si)) {
        si->nodes++;
        return 0;
    }

    ply=si->ply;
    if (ply > MAXPLY - 1) {
        return evaluate(board);
    }

    if (depth < 1) {
        return quiesce(board, alpha, beta, msp, si);
    }

    eval = evaluate(board);

    if (depth == 1 && eval <= alpha - 300) {
        return quiesce(board, alpha, beta, msp, si);
    }

    if (depth <= 7 && abs(beta) < 19000) {
        int margin = 120 * depth;
        if (eval - margin >= beta) {
            return beta;
        }
    }

    si->nodes++;

    if ((score = probe_tt(board,ply,depth,alpha,beta,&pvmove)) != INVALID) {
        if (!is_pv || (score > alpha && score < beta)) {
            return score;
        }
    }

    enumSide side = board->side;
    in_check = is_square_attacked(board,bitscanForward(board->bitboards[K+6*side]),1^side);

    if (depth >= 3 && !in_check && ply) {
        make_null(board, &undo);
        score = -search(board, depth-1-NULLMOVE_REDUCTION, -beta, -beta+1, msp, si, 0);
        unmake_null(board,&undo);
        if (score >= beta) {return beta;}
    }

    nmov = generate_moves(board, msp);
    si->msp[ply+1] = mp+nmov;
    score_moves(board, si, mp, nmov, pvmove);   //si->pv[0][ply]

    /* if (si->follow_pv) {
        score_pv(board,si,mp,nmov);
    } */
    
    tried=0;
    raised_alpha=false;

    while ((move = pick_move(mp++,nmov--)) != NOMOVE) {
        if (!make_move(board,move,&undo)) {continue;}
        tried++;
        si->ply++;

        // Reductions
        new_depth = depth - 1;
        reduction = 0;

        if (!is_pv
        && new_depth > 3
        && tried > 3
        && !IS_CAPTURE(move)
        && !IS_PROMOTION(move)
        && !in_check) {
            reduction = 1;
            if (tried > 6) {reduction++;}
            new_depth -= reduction;
        }

        if (!raised_alpha) {   // !raised_alpha
            score = -search(board, new_depth, -beta, -alpha, mp+nmov, si,is_pv);
        } else {
            if (reduction) {
                score = -search(board, new_depth, -alpha-1, -alpha, mp+nmov, si,false);
            } else {score = alpha + 1;}

            if (score > alpha) {
                new_depth += reduction;
                score = -search(board, new_depth, -alpha-1, -alpha, mp+nmov, si,false);
            }
            if ((score > alpha) && (score < beta)) {
                score = -search(board, new_depth, -beta, -alpha, mp+nmov, si,true);
            }
        }

        unmake_move(board,move,&undo);
        si->ply--;

        if (score > alpha) {
            alpha=score;
            bestmove = move;
            ttflag = TT_EXACT;
            raised_alpha=true;
            update_pv(si,move,ply);
            if (score >= beta) {
                if (!IS_CAPTURE(move)) {
                    si->killers[1][ply] = si->killers[0][ply];
                    si->killers[0][ply] = move;
                    if (board->last_move.piece != _) {
                        si->counter_moves[board->last_move.piece][board->last_move.to].piece = board->squares[MOVE_FROM(move)];
                        si->counter_moves[board->last_move.piece][board->last_move.to].to = MOVE_TO(move);
                    }
                    history_table[board->side][MOVE_FROM(move)][MOVE_TO(move)] += 1;    // depth*depth
                    if (history_table[board->side][MOVE_FROM(move)][MOVE_TO(move)] > SCORE_HIST_MAX) {half_history();}
                }
                ttflag = TT_BETA;
                alpha = beta;
                break;
            }
        }
    }

    if (tried==0) {
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
    score_moves(board, si, mp, nmov, si->bestmove); // si->bestmove

    /* if (si->follow_pv) {
        score_pv(board,si,mp,nmov);
    } */
    
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
                if (!IS_CAPTURE(move)) {
                    si->killers[1][ply] = si->killers[0][ply];
                    si->killers[0][ply] = move;
                    if (board->last_move.piece != _) {
                        si->counter_moves[board->last_move.piece][board->last_move.to].piece = board->squares[MOVE_FROM(move)];
                        si->counter_moves[board->last_move.piece][board->last_move.to].to = MOVE_TO(move);
                    }
                    history_table[board->side][MOVE_FROM(move)][MOVE_TO(move)] += 1; //depth*depth;
                    if (history_table[board->side][MOVE_FROM(move)][MOVE_TO(move)] > SCORE_HIST_MAX) {half_history();}
                }
                store_tt(board,depth,0,beta,TT_BETA,si->bestmove);
                info_pv(board,si);
                return beta;
            }
            alpha=score;
        }
    }

    store_tt(board,depth,0,alpha,TT_EXACT,si->bestmove);
    printf("info score cp %d depth %d nodes %ld pv ",score,depth,si->nodes);
    info_pv(board,si);

    return alpha;
}

int search_widen(board_t* board, int depth, int score, search_info_t* si) {
    int alpha,beta,val;
    alpha = score - 50;
    beta = score + 50;
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
    prepare_search(&si);
    si.ply = 0;
    si.nodes = 0L;
    si.bestmove = NOMOVE;

    depth = 1;
    score = search_root(board, 1, -INF, INF, &si);

    for (depth=2;depth<=MAXPLY;depth++) {
        if (time_control.stop || time_stop_root(&si)) {break;}
        best = si.bestmove;

        si.follow_pv = true;

        score = search_widen(board,depth,score,&si);
    }
    send_move(best);
}

void search_position(board_t* board) {
    time_calc();
    age_history();
    iterative_deepening(board);
}