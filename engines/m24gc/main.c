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
}


int main() {
    int debug = 1;
    init_all();
    if (debug) {
        parse_fen(kiwipete);
        search(6);
    } else {
        uci();
    }
}