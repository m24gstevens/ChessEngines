#include "gauss.h"
#include "bitboard.h"
#include "common.h"
#include "board.h"
#include "moves.h"
#include "uci.h"

int main() {
    init_attack_tables();
    board_t board;
    int debug=0;
    if (debug) {
        int i,n;
        long nodes,timer;
        board_t board;
        //parse_position(&board,"position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -  moves e2a6 h3g2 e1c1 g2g1N");
        print_board(board);
    }
    uci_loop(&board);
}