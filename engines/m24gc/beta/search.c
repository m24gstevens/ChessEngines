#include "defs.h"
#include "data.h"
#include "protos.h"

// MVV LVA: 13 victims, as there may be an empty victim, signalling an en-passent capture
// [victim][attacker]
int MVV_LVA[13][12] = {
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

    10,15,14,13,12,11, 10,15,14,13,12,11        // "Empty" target is when the target capture square is empty (For en-passent)
};

// killer heuristic
// [id][ply];
int killer_moves[2][MAX_PLY];

// history heuristic
// [piece][to]
int history_moves[12][64];

// Counter heuristic
// [piece][to]
U16 counter_moves[12][64];


void clear_tables() {
    memset(killer_moves, 0, sizeof(killer_moves));
    memset(counter_moves, 0, sizeof(counter_moves));
    memset(pv_length, 0, sizeof(pv_length));
    memset(pv_table, 0, sizeof(pv_table));
    memset(history_moves, 0, sizeof(history_moves));
}

void adjust_history() {
    for (int i=0; i<12; i++) {
        for (int j=0; j<64; j++)
            history_moves[i][j] /= 2;
    }
}

// PV move scoring
static inline void score_pv() {
    follow_pv = _FALSE;
    for (int i=moves_start_idx[ply]; i < moves_start_idx[ply+1]; i++) {
        if (pv_table[0][ply] == move_stack.moves[i].move) {
            follow_pv = _TRUE;
            move_stack.moves[i].score += 20000 + 80000000;
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
    int non_history_bonus = 80000000;
    if (move_flags(move) & 0x4) {
        int SEE_bonus = 0;
        // Score by SEE
        int SEE_value = SEE(side_to_move, move);
        if (SEE_value > 0)
            SEE_bonus = 2000;
        else if (SEE_value == 0)
            SEE_bonus = 1000;
        else
            SEE_bonus = -7000;
        //Score captures by MVV LVA
        return MVV_LVA[piece_on_square[move_target(move)]][piece_on_square[move_source(move)]] + SEE_bonus + 10000 + non_history_bonus;
    } else {
        //Score quiet
        //score killer moves
        if (killer_moves[0][ply] == move)
            return 9000 + non_history_bonus;
        else if (killer_moves[1][ply] == move)
            return 8000 + non_history_bonus;
        // Counter moves
        else if (last_move.piece != EMPTY) {
            if (counter_moves[last_move.piece][last_move.to] == move)
                return 7000 + non_history_bonus;
        }
        //score by history
        else {
            int pc = piece_on_square[move_source(move)];
            int target = move_target(move);
            return history_moves[piece_on_square[move_source(move)]][move_target(move)];
        }
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
void score_moves(U16 best_move) {
    int i;
    for (int i=moves_start_idx[ply]; i<moves_start_idx[ply+1]; i++) {
        U16 move = move_stack.moves[i].move;
        if (best_move == move) {
            move_stack.moves[i].score = 40000 + 80000000;
        }
        else
            move_stack.moves[i].score = score_move(move);
    }
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
    // Check up on search
    if ((nodes & 2047) == 0)
        communicate();
    pv_length[ply] = ply;
    nodes++;

    int static_evaluation = eval(alpha, beta);
    if (static_evaluation >= beta) {
        // Fail high
        return beta;
    }
    // initial prune for really hopeless positions
    int big_delta = 975;
    if (promoting_ranks[side_to_move] & bitboards[P + 6*side_to_move])
        big_delta += 775;
    if (static_evaluation + big_delta < alpha)
        return alpha;

    if (static_evaluation > alpha) {
        // PV node
        alpha = static_evaluation;
    }
    // Consider only the captures
    generate_captures();
    // Move-ordering
    score_moves(0);
    // PV scoring
    if (follow_pv)
        score_pv();
    sort_moves();
    for (int i=moves_start_idx[ply]; i<moves_start_idx[ply+1];i++) {
        U16 move = move_stack.moves[i].move;
        // Delta pruning - If static eval + captured piece value + delta margin < alpha, we can skip
        // Don't use in the case of promotions (Should be turned off in the endgame)
        if (!(move_flags(move) & 0x8) && static_evaluation + positive_simple_piece_values[piece_on_square[move_target(move)]] + DELTA < alpha)
            continue;
        // If this capture has a SEE value < 0, discard
        if (positive_simple_piece_values[piece_on_square[move_source(move)]] > positive_simple_piece_values[piece_on_square[move_target(move)]]
        && SEE(side_to_move, move) < 0)
            continue;
        make_move(move_stack.moves[i].move);
        int score = -quiesce(-beta, -alpha);
        unmake_move();

        // if time is up, return 0
        if (stopped == 1) return 0;

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

// Late move reduction constants
const int full_depth_moves = 4;
const int reduction_limit = 3;

// Negamax with alpha-beta search
int negamax(int depth, int alpha, int beta) {
    // initialize PV length
    pv_length[ply] = ply;

    int score;

    U16 best_move = 0;  // To be stored in transposition table

    // Check the number of repetitions. If we have repeated the position before, and we aren't at the root,
    // We can assume that the position is a draw (If one side has better, it will be cutoff
    if (ply && reps())
        return 0;         // Contempt factor

    int hash_flag = HASH_FLAG_ALPHA;

    int pv_node = beta - alpha > 1;     // Tests if we are at a PV Node

    // check if we have already searched this node by a tt lookup, if we are neither at the root nor at a PV node
    if (ply && (score = probe_hash(depth, &best_move, alpha, beta)) != NO_HASH_ENTRY && !pv_node)
        return score;

    if ((nodes & 2047) == 0)
        communicate();

    if (depth == 0)
        return quiesce(alpha, beta);
    //If we are too deep
    if (ply > MAX_PLY - 1)
        return eval(alpha, beta);
    
    nodes++;

    int legal_moves = 0;
    
    U16 move;
    // If we are in check, search deeper
    U64 checkers = all_checkers();
    if (checkers)
        depth++;
    
    int static_evaluation = eval(alpha, beta);
    // Static null move
    if (depth <= 3 && !checkers && !pv_node && abs(beta) < 50000 - 100) {
        int margin = 120 * depth;
        if (static_evaluation - margin >= beta)
            return beta;
    }

    // Null-move pruning
    if (depth >= 3 && !checkers && ply) {
        // Make a "Null" move
        make_null();
        // Null move search to find a beta cutoff)
        score  = -negamax(depth - 1 - NULLMOVE_R, -beta, -beta+1);
        // Unmake "Null" move
        unmake_null();
        // Failhard beta cutoff
        if (score >= beta)
            return beta;
    }

    // Razoring
    if (depth == 1 && static_evaluation <= alpha - 300) {
        return quiesce(alpha, beta);
    } 
    // Decide if futility pruning is applicaple
    int lmpMargin[5] = { 0, 5, 8, 12, 17};
    int futility_margin[4] = {0, 200, 300, 500};
    int futility = 0;
    if (depth <= 3 && !pv_node && !checkers && abs(alpha) < 48000 && (static_evaluation + futility_margin[depth] <= alpha))
        futility = 1;
    generate_moves();
    // Move-ordering
    score_moves(best_move);
    // PV Scoring
    if (follow_pv)
        score_pv();
    sort_moves();
    // Number of moves searched at current depth
    int moves_searched = 0;
    int quiet_searched = 0;
    for (int i=moves_start_idx[ply]; i<moves_start_idx[ply+1];i++) {
        move = move_stack.moves[i].move;
        make_move(move);
        legal_moves++;
        if (!pv_node && moves_searched && !(move_flags(move) & 0xC) & !in_check(side_to_move)) {
            // Futility pruning
            if (futility) {
                unmake_move();
                continue;
            }
            //Late move pruning
            if (depth <= 4 && quiet_searched > lmpMargin[depth] && fifty_move > 0) { //Enough quiet moves and not a pawn move
                unmake_move();
                continue;
            }

        }
        // All other nodes
        int new_depth = depth - 1;
        int reduction_depth = 0;
        // Late move reduction
        if (moves_searched >= full_depth_moves && depth >= reduction_limit &&
            !checkers && !(move_flags(move) & 0xC)) {
            reduction_depth = 1 + sqrt((double)(depth - 1)) + sqrt((double)(moves_searched - 1))/2;
            if (pv_node)
                reduction_depth = (reduction_depth * 2) / 3;
            // Reduce killers and counters less
            if (move_stack.moves[i].score > 80000000)
                reduction_depth--;
            reduction_depth = MIN(new_depth - 1, MAX(0, reduction_depth));  // Don't drop to Qsearch
            new_depth -= reduction_depth;
        }
        if (moves_searched == 0)
            score = -negamax(new_depth, -beta, -alpha);
        else {
            // Possibly a reduced search, null window
            score = -negamax(new_depth, -alpha - 1, -alpha);
            // If LMR failed, try a null search with full depth
            if (reduction_depth && score > alpha) {
                new_depth += reduction_depth;
                score = -negamax(new_depth, -alpha - 1, -alpha);
            }
            // if PVS fails, search with a full depth on the original alpha-beta bounds
            if ((score > alpha) && (score < beta))
                score = -negamax(new_depth, -beta, -alpha);
        }
        unmake_move();
        moves_searched++;
        if (!(move_flags(move) & 0xC))
            quiet_searched++;
        // return 0 if time is up
        if (stopped == 1) return 0;

        if (score > alpha) {
            // store hash entry with flag of PV
            hash_flag = HASH_FLAG_EXACT;
            // Store the best move (For TT)
            best_move = move;
            // PV node
            alpha = score;
            // write PV move
            pv_table[ply][ply] = move;
            // copy from deeper ply
            for (int next_ply = ply + 1; next_ply < pv_length[ply + 1]; next_ply++)
                pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];
            // adjust PV length
            pv_length[ply] = pv_length[ply + 1];
        }

        if (score >= beta) {
            // store hash entry with flag of beta cutoff
            record_hash(depth, best_move, beta, HASH_FLAG_BETA);

            if (!(move_flags(move) & 0x4)) {
                // Store killer moves
                killer_moves[1][ply] = killer_moves[0][ply];
                killer_moves[0][ply] = move;
                // Counter moves
                if (last_move.piece != EMPTY)   // There was a last move
                    counter_moves[last_move.piece][last_move.to] = move;
                history_moves[piece_on_square[move_source(move)]][move_target(move)] += depth * depth;
                if (history_moves[piece_on_square[move_source(move)]][move_target(move)] > 80000000)
                    adjust_history();
            }
            // Fail high
            return beta;
        }

    }
    if (legal_moves == 0) {
        if (checkers)
            return -49000 + ply;
        else
            return 0;
    }
    // Fifty move rule draw
	if (fifty_move >= 100)
		return 0;
    // Write the hash entry with score = alpha, using the best ,pve
    record_hash(depth, best_move, alpha, hash_flag);
    return alpha;
}

void search(int depth) {
    int score;
    int alpha, beta;
    U16 best_found = 0;
    // Initial alpha beta bounds
    alpha = -50000;
    beta = 50000;
    //Prepare the search
    nodes=0;
    clear_tables();
    // reset "time is up" flag
    stopped = 0;

    for (int current_depth = 1; current_depth <= depth; current_depth++) {
        follow_pv = 1;
        score = negamax(current_depth,alpha,beta);
        if ((score <= alpha) || (score >= beta)) {
            // Fell outside the window; try again with a full-width window, same depth
            alpha = -50000;
            beta = 50000;
            score = negamax(current_depth,alpha,beta);
        }
        alpha = score - VALWINDOW;
        beta = score + VALWINDOW;

        // if time is up
        if (stopped == 1)
            break;

        if (pv_length[0]) {
        printf("info score cp %d depth %d nodes %lld pv ",score,current_depth,nodes);
        print_pv();
        printf("\n");
        }
        best_found = pv_table[0][0];
    }
    ply = 0;
    printf("bestmove ");
    print_move(best_found);
    printf("\n");
    return;
}

void quickgame_search(quickresult_t *result) {
    int score;
    int alpha, beta;
    U16 best_found = 0;
    // Initial alpha beta bounds
    alpha = -50000;
    beta = 50000;
    //Prepare the search
    nodes=0;
    clear_tables();
    // reset "time is up" flag
    stopped = 0;

    for (int current_depth = 1; current_depth <= 32; current_depth++) {
        follow_pv = 1;
        score = negamax(current_depth,alpha,beta);
        if ((score <= alpha) || (score >= beta)) {
            // Fell outside the window; try again with a full-width window, same depth
            alpha = -50000;
            beta = 50000;
            score = negamax(current_depth,alpha,beta);
        }
        alpha = score - VALWINDOW;
        beta = score + VALWINDOW;

        // if time is up
        if (stopped == 1)
            break;

        if (pv_length[0]) {
        }
        best_found = pv_table[0][0];
    }
    result->bestmove = best_found;
    result->evaluation = score;
}