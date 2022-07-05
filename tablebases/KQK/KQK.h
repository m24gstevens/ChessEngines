#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
typedef uint32_t U32;
typedef uint64_t U64;
#define C64(constantU64) constantU64##ULL
typedef uint16_t U16;
#define MAX_MOVES 36
typedef struct {
    U16 positions[MAX_MOVES];
    int length;
} pos_list;
#define TRUE 1
#define FALSE 0
#define NUM_POSITIONS 0xFFFF
#define _ak(p) (p & 0x3F)
#define _aq(p) ((p >> 6) & 0x3F)
#define _dk(p) ((p >> 12) & 0xF)
#define BITSCAN(bb) __builtin_ffsll(bb)
#define positive_ray(bb, sq) (bb & (C64(-2) << sq))
#define negative_ray(bb, sq) (bb & ((C64(1) << sq) - 1))

void print_bb(U64);
void print_u16(U16);
U64 north_one(U64 bb);
U64 north_one(U64 bb);
U64 east_one(U64 bb);
U64 west_one(U64 bb);
U64 rank_mask(char sq);
U64 file_mask(char sq);
U64 diagonal_mask(char sq);
U64 antidiagonal_mask(char sq);
U64 king_attack_set(char);

int naive_ms1b(int i);
int bitscan_reverse(U64 bb, char table[0xFF]);

void init_ray_attacks(U64 attacks[8][64]);
/* gen_queen_attack_tables: Given a set of ray attacks and ms1b table, generates the attack table for the queen in that position */
void gen_queen_attack_tables(U64 ray_attacks[8][64], char ms1bTable[0xFF], U64 table[64][64]);

/* Generates all possible white moves from a given position. 
Only legality check is for whether pieces are atop one another, or kings or adjacent */
void gen_white_moves(U16 p, U64 queen_attack_set, pos_list *pl);

/* Generates all possible black moves from a given position. 
Only legality check is for whether pieces are atop one another, or kings or adjacent */
void gen_black_ancestors(U16 p, pos_list *pl);

/* gen_white_parents: Given a position with black to move, generates all legal parent positions with white to move */
void gen_white_parents(U16 p, U64 queen_attack_table[64][64], pos_list *pl);

/* gen_white_children: Given a position with black to move, generates all legal child positions with white to move */
void gen_white_children(U16 p, U64 queen_attack_table[64][64], pos_list *pl);

char bitarray_get(char bitarray[], long idx);
void bitarray_set(char bitarray[], long idx);
void bitarray_clear(char bitarray[], long length);

/* normalize_square: Given a square index, and details of a transformation to take the
black king to a particular set of 10 squares, performs the square transformation on a given index */
char normalize_square(char sq, int is_left, int is_bottom, int is_below_diag);

int king_adjacent(char a, char b);
int rook_adjacent(char a, char b);

/* Legality in that kings can't be adjacent and pieces aren't atop each other */
int is_legal(U16 p);
int is_checkmate(U16 p, U64);
char checkmate_flags(U16 p, U64);
int is_check(U16 p, U64 attack_set);

int create_file(char *data, long length, char *filename);