#include "gauss.h"
#include "bitboard.h"
#include "common.h"
#include "board.h"
#include "moves.h"
#include "uci.h"

int main() {
    init_tables();
    board_t board;
    board.last_move.piece = _;
    board.last_move.to = a1;
    int debug=0;
    if (debug) {
        parse_position(&board,"position fen r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 ");
        parse_go(&board,"go movetime 5000");
        return 0;
    }
    uci_loop(&board);
}