#include "gauss.h"
#include "bitboard.h"
#include "common.h"
#include "board.h"
#include "moves.h"
#include "uci.h"
#include "see.h"
#include "eval.h"
#include "order.h"

int main() {
    int n;
    init_tables();
    board_t board;
    setup_board(&board);
    long tim;
    int debug=0;
    if (debug) {
        tim = get_time_ms();
        parse_position(&board, "position startpos");
        parse_go(&board, "go depth 12");
        printf("\n Time ms: %d\n", get_time_ms() - tim);
        return 0;
    }
    uci_loop(&board);
}