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
        parse_position(&board,"position startpos");
        printf("%d\n",evaluate(&board));
        return 0;
    }
    uci_loop(&board);
}