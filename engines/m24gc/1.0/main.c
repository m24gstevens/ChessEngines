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
    }
    hply=0;
    clear_move_stack();
}

void prepare_search() {
    move_stack.count = 0;
    moves_start_idx[0] = 0;
    nodes=0;
    clear_tables();
};

void init_all() {
    ply = 0;
    prepare_search();
    init_jumper_attack_masks();
    init_slider_attack_masks();
    init_random_keys();
    //clear_hash();
}


int main() {
    int debug = 0;
    init_all();
    init_times();
    if (debug) {
        parse_fen("1r2k2r/pp1bnp1p/2p1p1p1/4P3/q3BB1Q/PR6/2P2PPP/5RK1 w k - 1 17");
        parse_go("go infinite");
        //printf("%d\n", eval());
    } else {
        uci();
    }
}