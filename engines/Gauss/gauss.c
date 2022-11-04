#include "gauss.h"
#include "bitboard.h"
#include "common.h"
#include "board.h"
#include "moves.h"

int main() {
    init_attack_tables();
    int debug=1;
    if (debug) {
        int ct;
        board_t board;
        parse_fen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 ",&board);
        print_board(board);

        move_t moves[100];
        ct = generate_moves(&board, moves);

        for (int i=0; i<ct; i++) {
            print_move(moves[i].move);
            printf("\n");
        }

        printf("\n%d\n",ct);
        printf("\n%d\n", board.squares[17]);

        print_bitboard(board.occupancies[BLACK]);

    }
}