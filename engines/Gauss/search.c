#include "search.h"
#include "eval.h"

move_t best_move;
int ply;

static inline int quiesce(board_t* board, int alpha, int beta) {
    int in_check, score, nmov, i, ct, stand_pat;
    move_t best_sofar;
    int old_alpha = alpha;

    stand_pat = evaluate(board);

    if (stand_pat >= beta) {return beta;}

    if (stand_pat > alpha) {
        alpha=stand_pat;
    }

    move_t move_list[200];
    hist_t undo;
    nmov = generate_moves(board, move_list);

    for (int i=0; i<nmov; i++) {
        if (!(MOVE_FLAGS((move_list+i)->move) & 0x4) || !make_move(board,move_list+i,&undo)) {continue;}
        ply++;
        score = -quiesce(board,-beta,-alpha);
        unmake_move(board,move_list+i,&undo);
        ply--;

        if (score >= beta) {return beta;}

        if (score > alpha) {
            alpha=score;
            if (ply==0) {
                best_sofar.move = (move_list+i)->move;
            }
        }
    }

    if (old_alpha != alpha) {
        best_move.move = best_sofar.move;
    }

    return alpha;
}

int negamax(board_t* board, int depth, int alpha, int beta) {
    int score,nmov,ct,i,in_check,legal;
    move_t best_sofar;
    int old_alpha = alpha;

    if (depth==0) {
        return quiesce(board,alpha,beta);
    }

    enumSide side = board->side;
    in_check = is_square_attacked(board,bitscanForward(board->bitboards[K+6*side]),1^side);

    move_t move_list[200];
    hist_t undo;
    nmov = generate_moves(board, move_list);
    legal=0;
    for (int i=0; i<nmov; i++) {
        if (!make_move(board,move_list+i,&undo)) {continue;}
        ply++;
        score = -negamax(board,depth-1,-beta,-alpha);
        legal++;
        unmake_move(board,move_list+i,&undo);
        ply--;

        if (score >= beta) {return beta;}

        if (score > alpha) {
            alpha=score;
            if (ply==0) {
                best_sofar.move = (move_list+i)->move;
            }
        }
    }

    if (legal==0) {
        return in_check ? -49000+ply : 0;
    }

    if (old_alpha != alpha) {
        best_move.move = best_sofar.move;
    }

    return alpha;
}

void search_position(board_t* board, int depth) {
    ply=0;
    int score = negamax(board,depth,-50000,50000);
    printf("bestmove ");
    print_move(best_move.move);
    printf("\n");
}