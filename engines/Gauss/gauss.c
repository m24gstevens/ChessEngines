#include "gauss.h"
#include "bitboard.h"
#include "common.h"
#include "board.h"
#include "moves.h"

int main() {
    init_attack_tables();
    int debug=1;
    if (debug) {
        int i,n;
        long nodes,timer;
        board_t board;
        parse_fen(starting_position,&board);
        print_board(board);

        move_t move[10000];
        hist_t undo[12];

        for (int i=1;i<7;i++) {
            timer = get_time_ms();
            nodes = perft(&board,move,undo,i);
            printf("Depth %d Nodes %ld Timed %ldms\n",i,nodes,get_time_ms()-timer);
        }
    }
}