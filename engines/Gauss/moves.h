#ifndef _MOVE_H_
#define _MOVE_H_

#include "common.h"
#include "board.h"
#include <ctype.h>

typedef struct {
    U16 move;
    //int score;
} move_t;

typedef struct {
    U8 captured;
    U8 castling;
    short ep_square;
    int rule50;
} hist_t;

 /* Encode a move in a U16 as follows 
 high bits                      low bits
  0000 0000 0011 1111 - Move from
  0000 1111 1100 0000 - Move to
  1111 0000 0000 0000 - Move flags
  Flags go like this:
  0000 - Quiet move     0001 - Double pawn push     0010 - Kingside Castle    0011 - Queenside castle
  0100 - Capture        0101 - En Passent           10xx -  Promotion         11xx- Capture promotion
  Promotion bits go    00 for knight, 01 for bishop,  10 for rook,   11 for queen */

#define encode_move(move_from, move_to, move_flags) \
    ((move_from) & 0x3F) | (((move_to) << 6) & 0xFC0) | (((move_flags) << 12) & 0xF000)

#define move_from(move) ((move) & 0x3F)
#define move_to(move) ((move)>>6 & 0x3F)
#define move_flags(move) ((move)>>12 & 0xF)
#define is_promotion(move) ((move) & 0x8000)
#define move_promote_to(move) ((move)>>12 & 0x3)

extern char* square_strings[64];
extern char* promoted_pieces;

void print_move(U16);

int generate_moves(board_t*, move_t*);

int is_square_attacked(board_t*, enumSquare, enumSide);

#endif