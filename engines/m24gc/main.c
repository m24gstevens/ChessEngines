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
        parse_fen("rnbq1bnr/pp2pkpp/8/2pp4/8/4P3/PPPP1PPP/RNB1KBNR w KQ - 0 1");
        printf("Black king: %d\n", eval_king(BLACK, 2200));
        printf("White king: %d\n", eval_king(WHITE, 3100));
        //printf("%d\n", eval());
    } else {
        uci();
    }
}