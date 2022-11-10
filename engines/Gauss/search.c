#include "search.h"
#include "eval.h"
#include "order.h"
#include "uci.h"
#include "tt.h"
#include "see.h"
#include "board.h"

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

    if (ply = 0) {
        ;
    }
}

void score_pv(search_info_t* si, move_t* mp, int nmoves, int ply) {
    si->follow_pv = false;
    for (int i=0;i<nmoves;i++) {
        if ((mp+i)->move == si->pv[0][ply]) {
            si->follow_pv = true;
            (mp+i)->score = SCORE_PV;
            return;
        }
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

void debug_search(board_t* board, search_info_t* si, int depth, int ply) {
    printf("PV LINE\n");
    print_pv(si,depth);
    printf("\n\n si information \n\n");
    printf("PLY %d si->PLY %d \n SDEPTH %d \n NODES %ld\n BESTMOVE ",ply, si->ply, si->sdepth, si->nodes);
    print_move(si->bestmove);
    printf("\n");
}

int quiesce(board_t* board, search_info_t* si, int alpha, int beta) {
    int score, nmoves, ply, stand_pat, tried, eval;
    hist_t undo;
    move_t* mp;
    U16 move;

    if (alpha >= beta) {return alpha;}

    if (!(si->nodes & 2047)) {
        communicate(si);
    }
    if (time_control.stop) {return 0;}

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
    sort_moves(mp, nmoves);
    si->msp[ply+1] = mp + nmoves;

    if (si->follow_pv) {
        score_pv(si,mp,nmoves,ply);
    }

    while ((move = pick_move(mp++, nmoves--)) != NOMOVE) {
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



int search(board_t* board, search_info_t* si, int depth, int alpha, int beta, bool is_null) {
    int score, ct, nmoves, ply, in_check, tried, legal, reduction, new_depth, eval;
    bool pvnode, found_pv = false;
    hist_t undo;
    move_t* mp;
    U16 move, pvmove=NOMOVE, bestmove=NOMOVE;
    U8 hashflag = HASH_FLAG_ALPHA;

    pvnode = ((beta - alpha) > 1);

    ply = si->ply;

    if (alpha >= beta) {return alpha;}

    if (ply && is_draw(board,si)) {
        si->nodes++;
        return 0;
    }

    if (ply && (score = probe_tt(board, ply, depth, alpha, beta, &pvmove)) != INVALID) {
        return score;
    }

    if (!(si->nodes & 2047)) {
        communicate(si);
    }
    if (time_control.stop) {return 0;}


    if (depth == 0) {
        return quiesce(board, si, alpha, beta);
    }

    if (depth > MAXPLY - 1) {
        return evaluate(board);
    }

    si->nodes++;

    in_check = is_square_attacked(board, bitscanForward(board->bitboards[K + 6*board->side]), 1^board->side);
    if (in_check) {depth++;}

    eval = evaluate(board);

    // Razoring

    if (depth == 1 && eval <= alpha - 300) {
        return quiesce(board, si, alpha, beta);
    }

    // Static null-move

    if (depth <= 7 && abs(beta) < 19000) {
        int margin = 96 * depth;
        if (eval - margin >= beta) {
            return beta;
        }
    }

    // Null-move pruning

    if (is_null && depth >= 3 && !in_check && ply) {
        new_depth = depth - 1;
        reduction = (depth >= 8) ? 3 : 2; 
        new_depth -= reduction;
        make_null(board,&undo);
        si->msp[ply+1] = si->msp[ply];
        si->ply++;
        score = -search(board, si, new_depth, -beta, -beta+1, true);
        si->ply--;
        unmake_null(board,&undo);

        if (score >= beta) {
            return beta;
        }
    } 

    // Internal iterative deepening
    if ((pvmove == NOMOVE) && pvnode && depth >= 6) {
        new_depth = depth - (depth/4) - 1;
        score = search(board, si, new_depth, alpha, beta, is_null);
        if ((score > alpha) && (score < beta)) {
            pvmove = si->pv[ply][ply];
        }
    }

    mp = si->msp[ply];
    nmoves = generate_moves(board, mp);
    score_moves(board, si, mp, nmoves, pvmove);
    //sort_moves(mp, nmoves);
    si->msp[ply+1] = mp + nmoves;

    if (si->follow_pv) {
        score_pv(si,mp,nmoves,ply);
    }

    //if (ply == 0) {print_moves(board,mp,nmoves);}

    tried = 0;
    legal = 0;
    while ((move = pick_move(mp++, nmoves--)) != NOMOVE) {
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
            score = -search(board, si, new_depth, -beta, -alpha,true);
        } else {
            score = -search(board, si, new_depth, -alpha-1, -alpha,true);

            if (reduction && score > alpha) {
                new_depth += reduction;
                score = -search(board, si, new_depth, -alpha-1, -alpha,true);
            }

            if ((score > alpha) && (score < beta)) {
                score = -search(board, si, depth-1, -beta, -alpha,true);
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
                if (!IS_TACTICAL(move)) {
                    scores_cutoff(board, si, move, depth);
                }
                alpha = beta;
                hashflag = HASH_FLAG_BETA;
                break;
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
    score = search(board, si, depth, alpha, beta,true);
    if (score <= alpha || score >= beta) {
        score = search(board, si, depth, alpha, beta,true);
    }

    return score;
}

void iterative_deepening(board_t* board, search_info_t* si) {
    int score, depth;
    U16 best_move;
    board_t bc;

    bc = *board;

    si->sdepth = 1;
    score = search(board,si,1,-INF,INF,true);
    best_move = si->pv[0][0];

    for (depth=2; depth<MAXPLY; depth++) {
        //printf("DEPTH %d\n\n",depth);
        //print_board(*board);
        //printf("\n\n");

        if (time_control.stop || time_stop_root(si)) {break;}
        best_move = si->pv[0][0];

        printf("info score cp %d depth %d nodes %ld pv ", score, depth-1, si->nodes);
        print_pv(si,depth-1);
        printf("\n");

        si->sdepth = depth;
        score = aspiration_window(board,si,depth,score);
        *board = bc;
    }

    send_move(best_move);
}

void search_position(board_t* board) {
    int score;
    search_info_t si;
    move_t movestack[MAXMOVES];

    prepare_search(&si,&movestack[0]);
    time_calc(board);
    age_tt();

    iterative_deepening(board, &si);
}