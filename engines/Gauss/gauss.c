#include "gauss.h"
#include "bitboard.h"
#include "common.h"
#include "board.h"
#include "moves.h"
#include "uci.h"
#include "see.h"
#include "eval.h"

int main() {
    init_tables();
    board_t board;
    setup_board(&board);
    int debug=0;
    if (debug) {
        parse_position(&board,"position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ");
        parse_go(&board,"go movetime 20000");
        return 0;
    }
    uci_loop(&board);
}