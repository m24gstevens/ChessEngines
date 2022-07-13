#include "defs.h"
#include "data.h"
#include "protos.h"

// [victim][attacker]
int MVV_LVA[12][12] = {
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
    50,55,54,53,52,51, 50,55,54,53,52,51
};

// killer heuristic
// [id][ply];
int killer_moves[2][MAX_PLY];

// history heuristic
// [piece][square]
int history_moves[12][64];

// Principal variation scoring
int pv_length[MAX_PLY];
U16 pv_table[MAX_PLY][MAX_PLY];

// follow PV flag
int follow_pv;

void clear_tables() {
    memset(killer_moves, 0, sizeof(killer_moves));
    memset(history_moves, 0, sizeof(history_moves));
    memset(pv_length, 0, sizeof(pv_length));
    memset(pv_table, 0, sizeof(pv_table));
}

// PV move scoring
static inline void score_pv() {
    follow_pv = _FALSE;
    for (int i=moves_start_idx[ply]; i < moves_start_idx[ply+1]; i++) {
        if (pv_table[0][ply] == move_stack.moves[i].move) {
            follow_pv = _TRUE;
            move_stack.moves[i].score += 100000;
            return;
        }
    }
}

void print_pv() {
    for (int count=0; count < pv_length[0]; count++) {
        print_move(pv_table[0][count]);
        printf(" ");
    }
}

int score_move(U16 move) {
    if (move_flags(move) & 0x4) {
        //Score captures by MVV LVA
        return MVV_LVA[piece_on_square[move_target(move)]][piece_on_square[move_source(move)]] + 10000;
    } else {
        //Score quiet
        //score killer moves
        if (killer_moves[0][ply] == move)
            return 9000;
        else if (killer_moves[1][ply] == move)
            return 8000;
        //score by history
        else
            return history_moves[piece_on_square[move_source(move)]][move_target(move)];
    }
}

void print_move_scores() {
    for (int i=moves_start_idx[ply]; i<moves_start_idx[ply+1]; i++) {
        U16 move = move_stack.moves[i].move;
        printf("Move: ");
        print_move(move);
        printf(" Score: %d\n", score_move(move));
    }
}

// Scores the moves generated at the current ply
void score_moves() {
    int i;
    for (int i=moves_start_idx[ply]; i<moves_start_idx[ply+1]; i++)
        move_stack.moves[i].score = score_move(move_stack.moves[i].move);
}

// Sorts the moves at the current ply, higher scoring moves able to be searched first. Bubble sort
void sort_moves() {
    for (int current=moves_start_idx[ply]; current<moves_start_idx[ply+1];current++) {
       for (int next=current+1; next<moves_start_idx[ply+1];next++) {
         if (move_stack.moves[current].score < move_stack.moves[next].score) {
            // XOR swap algorithm
            move_stack.moves[current].move ^= move_stack.moves[next].move;
            move_stack.moves[current].score ^= move_stack.moves[next].score;
            move_stack.moves[next].move ^= move_stack.moves[current].move;
            move_stack.moves[next].score ^= move_stack.moves[current].score;
            move_stack.moves[current].move ^= move_stack.moves[next].move;
            move_stack.moves[current].score ^= move_stack.moves[next].score;
         }
       }
    }
}
// Quiescence search
int quiesce(int alpha, int beta) {
    pv_length[ply] = ply;
    nodes++;

    int static_evaluation = eval();
    if (static_evaluation >= beta) {
        // Fail high
        return beta;
    }
    if (static_evaluation > alpha) {
        // PV node
        alpha = static_evaluation;
    }

    generate_moves();
    // Move-ordering
    score_moves();
    // PV scoring
    if (follow_pv)
        score_pv();
    sort_moves();
    for (int i=moves_start_idx[ply]; i<moves_start_idx[ply+1];i++) {
        if (make_capture(move_stack.moves[i].move) == _FALSE)
            continue;
        int score = -quiesce(-beta, -alpha);
        unmake_move();

        if (score >= beta) {
            // Fail high
            return beta;
        }
        if (score > alpha) {
            // PV node
            alpha = score;
            pv_table[ply][ply] = move_stack.moves[i].move;
            // copy from deeper ply
            for (int next_ply = ply + 1; next_ply < pv_length[ply + 1]; next_ply++)
                pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];
            // adjust PV length
            pv_length[ply] = pv_length[ply + 1];
        }
    }
    // Fail low
    return alpha;
}

// Negamax with alpha-beta search
int negamax(int depth, int alpha, int beta) {
    int found_pv = 0;
    // initialize PV length
    pv_length[ply] = ply;

    if (depth == 0)
        return quiesce(alpha, beta);
    //If we are too deep
    if (ply > MAX_PLY - 1)
        return eval();
    
    nodes++;

    int legal_moves = 0;
    int score;
    U16 move;
    // If we are in check, search deeper
    int checked = in_check(side_to_move);
    if (checked)
        depth++;

    generate_moves();
    // Move-ordering
    score_moves();
    // PV Scoring
    if (follow_pv)
        score_pv();
    sort_moves();
    for (int i=moves_start_idx[ply]; i<moves_start_idx[ply+1];i++) {
        move = move_stack.moves[i].move;
        make_move(move);
        legal_moves++;
        // PV node hit
        if (found_pv) {
            // Search rest of the moves with the goal of proving they are all bad
            score = -negamax(depth-1, -alpha -1, -alpha);
            // Need to re-search this move
            if ((score > alpha) && (score < beta))
                score = -negamax(depth-1, -beta, -alpha);

        } else {
            score = -negamax(depth - 1, -beta, -alpha);
        }
        unmake_move();

        if (score >= beta) {
            if (!(move_flags(move) & 0x4)) {
                // Store killer moves
                killer_moves[1][ply] = killer_moves[0][ply];
                killer_moves[0][ply] = move;
            }
            // Fail high
            return beta;
        }
        if (score > alpha) {
            // store history move
            if (!(move_flags(move) & 0x4)) 
                history_moves[piece_on_square[move_source(move)]][move_target(move)] += depth;
            // PV node
            alpha = score;
            found_pv = 1;
            // write PV move
            pv_table[ply][ply] = move;
            // copy from deeper ply
            for (int next_ply = ply + 1; next_ply < pv_length[ply + 1]; next_ply++)
                pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];
            // adjust PV length
            pv_length[ply] = pv_length[ply + 1];
        }
    }
    if (legal_moves == 0) {
        if (checked)
            return -49000 + ply;
        else
            return 0;
    }
    // Fail low
    return alpha;
}

void search(int depth) {
    int score;
    //iterative deepening
    prepare_search();
    // reset "time is up" flag
    follow_pv = 0;

    for (int current_depth = 1; current_depth <= depth; current_depth++) {
        // if time is up
        follow_pv = 1;
        score = negamax(current_depth,-300000,300000);
        printf("info score cp %d depth %d nodes %lld pv ",score,current_depth,nodes);
        print_pv();
        printf("\n");
    }
    printf("bestmove ");
    print_move(pv_table[0][0]);
    printf("\n");

}