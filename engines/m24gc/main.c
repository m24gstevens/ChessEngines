#include "defs.h"
#include "data.h"
#include "protos.h"

void prepare_search() {
    move_stack.count = 0;
    moves_start_idx[0] = 0;
    ply = 0;
    nodes=0;
    clear_tables();
};

void init_all() {
    prepare_search();
    init_jumper_attack_masks();
    init_slider_attack_masks();
    init_random_keys();
    clear_hash();
}


int main() {
    int debug = 0;
    init_all();
    init_times();
    if (debug) {
        parse_fen(starting_position);
        search(10);
        // depth 10 nodes 1475600 pv e2e4 e7e5 b1c3 b8c6 g1f3 
        // depth 10 nodes 1513119 pv e2e4 e7e5 g1f3 g8f6 b1c3 f6e4 e5c6 
    } else {
        uci();
    }
}