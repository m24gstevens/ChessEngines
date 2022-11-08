#include "search.h"
#include "eval.h"
#include "order.h"
#include "uci.h"
#include "tt.h"
#include "see.h"

void prepare_search(search_info_t* si, move_t* base) {
    si->ply = 0;
    si->nodes = 0L;
    si->bestmove = NOMOVE;
    si->msp[0] = base;
    for (int i=0; i<MAXPLY; i++) {si->pv[i][0] = NOMOVE;}
    age_history_heuristic();
}

void update_pv(search_info_t* si, U16 move, int ply) {
    int i;
    U16 line;
    si->pv[ply][ply] = move;
    for (i=1;(line = si->pv[ply+1][ply+i]) != NOMOVE;i++) {
        si->pv[ply][ply+i] = line;
    }
}

void print_pv(search_info_t* si, int depth) {
    int i;
    U16 line;
    for (i=0; i<depth && ((line = si->pv[0][i]) != NOMOVE); i++) {
        print_move(line);
        printf(" ");
    }
}

int quiesce(board_t* board, search_info_t* si, int alpha, int beta) {
    int score, nmoves, ply, stand_pat, tried;
    hist_t undo;
    move_t* mp;
    U16 move;

    if (alpha >= beta) {return alpha;}

    if (!(si->nodes & 2047)) {
        communicate(si);
    }

    si->nodes++;

    stand_pat = evaluate(board);

    if (stand_pat >= beta) {
        return beta;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    ply = si->ply;

    mp = si->msp[ply];
    nmoves = generate_captures(board, mp);
    score_moves(board, si, mp, nmoves, NOMOVE);
    si->msp[ply+1] = mp + nmoves;

    tried = 0;
    while ((move = pick_move(mp++,nmoves--)) != NOMOVE) {
        tried++;
        if (!make_move(board,move,&undo)) {continue;}
        si->ply++;

        score = -quiesce(board, si, -beta, -alpha);

        unmake_move(board,move,&undo);
        si->ply--;

        if (time_control.stop) {return 0;}

        if (score > alpha) {
            alpha = score;

            if (score >= beta) {
                return beta;
            }
        }
    }

    return alpha;
}



int search(board_t* board, search_info_t* si, int depth, int alpha, int beta) {
    int score, ct, nmoves, ply, in_check, tried, legal, reduction, new_depth;
    bool found_pv = false;
    hist_t undo;
    move_t* mp;
    U16 move, pvmove=NOMOVE, bestmove=NOMOVE;
    U8 hashflag = HASH_FLAG_ALPHA;

    if (alpha >= beta) {return alpha;}

    if (!(si->nodes & 2047)) {
        communicate(si);
    }

    ply = si->ply;

    if (depth > MAXPLY - 1) {
        return evaluate(board);
    }

    if (depth == 0) {
        return quiesce(board, si, alpha, beta);
    }

    if ((score = probe_tt(board, ply, depth, alpha, beta, &pvmove)) != INVALID) {
        return score;
    }

    si->nodes++;

    in_check = is_square_attacked(board, bitscanForward(board->bitboards[K + 6*board->side]), 1^board->side);

    if (depth >= 3 && !in_check && ply) {
        make_null(board,&undo);
        si->ply++;
        score = -search(board, si, depth-1-NULLMOVE_REDUCTION, -beta, -beta+1);
        si->ply--;
        unmake_null(board,&undo);

        if (score >= beta) {
            return beta;
        }
    }

    mp = si->msp[ply];
    nmoves = generate_moves(board, mp);
    score_moves(board, si, mp, nmoves, pvmove);
    si->msp[ply+1] = mp + nmoves;

    tried = 0;
    legal = 0;
    while ((move = pick_move(mp++,nmoves--)) != NOMOVE) {
        tried++;
        if (!make_move(board,move,&undo)) {continue;}
        si->ply++;
        legal++;

        new_depth = depth-1;
        reduction = 0;
        if (found_pv
            && new_depth > 3
            && tried > 3
            && !IS_CAPTURE(move)
            && !IS_PROMOTION(move)
            && !in_check) {

                reduction = 1;
                if (tried > 6) {reduction++;}
                if ((move == si->killers[0][ply]) || (move == si->killers[1][ply])) {reduction--;}
                new_depth -= reduction;
        }

        if (!found_pv) {
            score = -search(board, si, new_depth, -beta, -alpha);
        } else {
            score = -search(board, si, new_depth, -alpha-1, -alpha);

            if (reduction && score > alpha) {
                new_depth += reduction;
                score = -search(board, si, new_depth, -alpha-1, -alpha);
            }

            if ((score > alpha) && (score < beta)) {
                score = -search(board, si, depth-1, -beta, -alpha);
            }
        }

        unmake_move(board,move,&undo);
        si->ply--;

        if (time_control.stop) {return 0;}

        if (score > alpha) {
            alpha = score;
            found_pv = true;
            bestmove = move;
            hashflag = HASH_FLAG_EXACT;

            update_pv(si, move, ply);

            if (score >= beta) {
                if (IS_QUIET(move)) {
                    scores_cutoff(board, si, move, depth);
                }
                alpha = beta;
                hashflag = HASH_FLAG_BETA;
                return beta;
            }
        }
    }

    if (!legal) {
        return (in_check ? -MATE_SCORE + ply : 0);
    }

    store_tt(board, depth, ply, alpha, hashflag, bestmove);

    return alpha;
}

static inline int aspiration_window(board_t* board, search_info_t* si, int depth, int score) {
    int alpha, beta;
    alpha = score - 75;
    beta = score + 75;
    score = search(board, si, depth, alpha, beta);
    if (score <= alpha || score >= beta) {
        score = search(board, si, depth, alpha, beta);
    }

    return score;
}

void iterative_deepening(board_t* board, search_info_t* si) {
    int score, depth;
    U16 best_move;

    si->sdepth = 1;
    score = search(board,si,1,-INF,INF);
    best_move = si->pv[0][0];

    for (depth=2; depth<MAXPLY; depth++) {
        if (time_control.stop || time_stop_root(si)) {break;}
        best_move = si->pv[0][0];

        printf("info score cp %d depth %d nodes %ld pv ", score, depth-1, si->nodes);
        print_pv(si,depth-1);
        printf("\n");

        si->sdepth = depth;
        score = aspiration_window(board,si,depth,score);
    }

    send_move(best_move);
}

void search_position(board_t* board) {
    int score;
    search_info_t si;
    move_t movestack[MAXMOVES];

    prepare_search(&si,&movestack[0]);
    time_calc(board);

    iterative_deepening(board, &si);
}