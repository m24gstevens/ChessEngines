#ifndef _UCI_H_
#define _UCI_H_
#include "common.h"
#include "moves.h"
#include "board.h"

enum tFlags {
    FTIME=1,
    FINC=2,
    FMOVESTOGO=4,
    FDEPTH=8,
    FNODES=16,
    FMATE=32,
    FMOVETIME=64,
    FINFINITE=128
};

#define MOVESTOGO 30
#define TIMEBUF 500

extern timeinfo_t time_control;

bool time_stop_root(search_info_t* si);
void communicate(search_info_t*);
void time_calc();


void send_move(U16);

void uci_loop(board_t*);

void parse_position(board_t*,char*);
void parse_go(board_t*,char*);


#endif