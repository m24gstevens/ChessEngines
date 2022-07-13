#ifndef M24GC_H
#define M24GC_H
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
#define C64(constantU64) constantU64##ULL
/* Magic numbers */
#define MAX_PLY 64
#define STACK_SIZE 16000
#define MAX_HIST 500


/* enumerations */
#ifndef _M24GC
#define _M24GC
enum {_FALSE, _TRUE};
enum {
    a1,b1,c1,d1,e1,f1,g1,h1,
    a2,b2,c2,d2,e2,f2,g2,h2,
    a3,b3,c3,d3,e3,f3,g3,h3,
    a4,b4,c4,d4,e4,f4,g4,h4,
    a5,b5,c5,d5,e5,f5,g5,h5,
    a6,b6,c6,d6,e6,f6,g6,h6,
    a7,b7,c7,d7,e7,f7,g7,h7,
    a8,b8,c8,d8,e8,f8,g8,h8,
};

enum {WHITE, BLACK, BOTH};

enum {K,P,N,B,R,Q,k,p,n,b,r,q,EMPTY};

enum {WCK=1, WCQ=2, BCK=4, BCQ=8};

#endif

/* Functions from main */
void prepare_search();
void init_all();

/* =================================
            Bitboards
 =================================== */

#define northOne(bb) (bb << 8)
#define southOne(bb) (bb >> 8)
#define northTwo(bb) (bb << 16)
#define southTwo(bb) (bb >> 16)
#define eastOne(bb) ((bb & ~C64(0x8080808080808080)) << 1)
#define westOne(bb) ((bb & ~C64(0x0101010101010101)) >> 1)
#define eastTwo(bb) ((bb & ~C64(0xc0c0c0c0c0c0c0c0)) << 2)
#define westTwo(bb) ((bb & ~C64(0x0303030303030303)) >> 2)
#define isolate_ls1b(bb) bb &= -bb
#define clear_ls1b(bb) bb &= ((bb)-1)
#define general_shift(bb, shft) ((shft < 0) ? (bb >> -shft) : (bb << shft))
#define set_bit(bb, idx) bb |= ((U64)1 << (idx))
#define get_bit(bb, idx) (bb & ((U64)1 << (idx))) ? TRUE : FALSE
#define clear_bit(bb, idx) bb &= ~((U64)1 << (idx))
#define BITSCAN(bb) (__builtin_ffsll(bb))

/* bit twiddles */
void print_bitboard(U64 bb);
int bitscanForward(U64 bb);
int popcount32(U32 bb);
int popcount(U64 bb);
int popcount_sparse(U64 bb);

/* jumper pieces */
void init_jumper_attack_masks();

/* pseudo-random number generator for magic bitboards */
typedef struct {
    U32 state;
} xorshift32_state;
U32 random_xor();
U64 random_u64();

/* Magic bitboards */
typedef struct {
    U64 mask;
    U64 magic;
} MagicInfo;

void generate_magic_numbers();

/* initialization of attack tables */
void init_jumper_attack_masks();
void init_slider_attack_masks();
U64 index_to_u64(int, int, U64);
/* Functions to get the attack masks */

U64 bishop_attack_mask(int, U64);
U64 rook_attack_mask(int, U64);
U64 kingAttacks(int sq);
U64 knightAttacks(int sq);
U64 pawnAttacks(int side, int sq);
U64 bishopAttacks(int sq, U64 occ);
U64 rookAttacks(int sq, U64 occ);
U64 queenAttacks(int sq, U64 occ);

U64 bishopPinAttacks(int sq, U64 occ);
U64 rookPinAttacks(int sq, U64 occ);

U64 rankfile_pinmask();
U64 diagonal_pinmask();

/* =================================
            Board representation
 =================================== */

/* Printing the board */
void print_board();
void parse_fen(char *fen);

/* =================================
           Move Generation
 =================================== */
 /* Encode a move in a U16 as follows 
 high bits                      low bits
  0000 0000 0011 1111 - Move from
  0000 1111 1100 0000 - Move to
  1111 0000 0000 0000 - Move flags
  Flags go like this:
  0000 - Quiet move     0001 - Double pawn push     0010 - Kingside Castle    0011 - Queenside castle
  0100 - Capture        0101 - En Passent           10xx -  Promotion         11xx- Capture promotion
  Promotion bits go    00 for knight, 01 for bishop,  10 for rook,   11 for queen */

 /* Encode history as follows:
  struct of:
    -Old position data as flags
high bits                   low bits
  0000 0000 0000 1111 - Piece captured on last move (Possibly EMPTY)
  0000 0000 1111 0000 - Old castling state information  
  0000 0001 0000 0000 - Old en passent legal bit
  1111 1100 0000 0000 - Old en passent target square
   -Fifty move clock
   -Hash value
  */

typedef struct {
    U16 move;
    int score;
} move_t;

typedef struct {
    move_t moves[STACK_SIZE];
    int count;
} _move_list;

typedef struct {
    U16 move;
    U16 flags;
    int fifty_clock;
    U64 hash;
} hist_t;

#define encode_move(move_from, move_to, move_flags) \
    ((move_from) & 0x3F) | (((move_to) << 6) & 0xFC0) | (((move_flags) << 12) & 0xF000)
    
#define encode_hist(prev_capture, prev_castling, prev_ep_legal, prev_ep_target) \
   ((prev_capture) & 0x3F) | ((prev_castling) << 4) | ((prev_ep_legal) << 8) | ((prev_ep_target) << 10)

#define move_source(mov) ((mov) & 0x3F)
#define move_target(mov) ((mov & 0xFC0) >> 6)
#define move_flags(mov) ((mov & 0xF000) >> 12)
#define hist_capture(mov) ((mov) & 0xF)
#define hist_castling(mov) ((mov & 0xF0) >> 4)
#define hist_ep_legal(mov) ((mov & 0x100) >> 8)
#define hist_ep_target(mov) (mov >> 10)

void print_move(U16);
void print_move_stack(int);

int is_square_attacked(int, int);
int in_check(int);
void generate_moves();
void generate_captures();
 
int make_move(U16);
int make_capture(U16);
void unmake_move();
/* perft */
void perft_test(int);
void divide(int);

/* =================================
        Evaulation
 =================================== */
 #define FLIP(sq) ((sq) ^ 56)
 int eval();

/* =================================
        Search
 =================================== */
 extern U64 nodes;
 void search(int);
 int negamax(int, int, int);
 int score_move(U16);
void score_moves();
void sort_moves();
 void print_move_scores();

/* =================================
         Global Data Structures
 =================================== */

/* Attack tables */
extern U64 pawn_attack_table[2][64]; /*[ side to move ][ square index ] */
extern U64 knight_attack_table[64];
extern U64 king_attack_table[64];
extern U64 queen_attack_table[64];

extern MagicInfo magicBishopInfo[64];
extern MagicInfo magicRookInfo[64];

extern U64 bishopAttackTable[64][512];  /* Second index by magic hash */
extern U64 rookAttackTable[64][4096];   /* Second index by magic hash */

/* Board representation */
extern U64 bitboards[12];
extern U64 occupancies[3];
extern char piece_on_square[64];
extern int side_to_move;
extern int castling_rights;
extern int en_passent_legal;
extern int en_passent_square;
extern int ply;
extern int game_depth;
extern U64 hash;

extern _move_list move_stack;
extern int moves_start_idx[MAX_PLY];
extern hist_t game_history[MAX_HIST];

extern int MVV_LVA[12][12];
extern int killer_moves[2][MAX_PLY];
extern int history_moves[12][64];
extern U16 pv_table[MAX_PLY][MAX_PLY];
extern int pv_length[MAX_PLY];
extern int follow_pv;
#endif