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
}


int main() {
    int debug = 1;
    init_all();
    init_times();
    if (debug) {
        parse_fen(kiwipete);
        test_hash(4);
        printf("Done\n");
    } else {
        uci();
    }
}