#ifndef _UCI_H
#define _UCI_H

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef _WIN64
    #include <windows.h>
#else
    #include <sys/time.h>
#endif
#include "m24gc.h"

// Fixing some compilation errors

extern char *starting_position;
extern char *kiwipete;
extern char *cmk_position;

extern char char_to_piece_code[];
extern char *square_strings[64];
extern char *promoted_pieces;

int get_time_ms();

U16 parse_move(char *);
void parse_position(char *);
void parse_go(char *);
void uci();

#endif