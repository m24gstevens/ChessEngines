#include "defs.h"
#include "data.h"
#include "protos.h"

void clear_move_stack() {
    move_t *old_move;
    memset(moves_start_idx, 0, sizeof(moves_start_idx));
    for (int i=0; i<move_stack.count; i++) {
        old_move = &move_stack.moves[i];
        old_move->move = 0;
        old_move->score = 0;
    }
    ply=0;
}

void clear_history() {
    int i;
    hist_t *old_hist;
    for (int i=0; i<MAX_HIST; i++) {
        old_hist = &game_history[i];
        old_hist->move=0;
        old_hist->flags=0;
        old_hist->fifty_clock=0;
        old_hist->hash = C64(0);
        old_hist->material[0] = 0;
        old_hist->material[1] = 0;
    }
    hply=0;
    clear_move_stack();
}

void prepare_search() {
    move_stack.count = 0;
    moves_start_idx[0] = 0;
    nodes=0;
    clear_tables();
}

void init_all() {
    ply = 0;
    prepare_search();
    init_jumper_attack_masks();
    init_slider_attack_masks();
    init_random_keys();
    init_evaluation_masks();
    // Initialize hash table with default 32MB
    init_hash_table(32);
}


int main() {
    int debug = 1;
    init_all();
    init_times();
    if (debug) {
        parse_fen(kiwipete);
        search(12);
    } else {
        uci();
    }
}