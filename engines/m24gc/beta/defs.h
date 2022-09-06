#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
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
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define DISTANCE(sq1, sq2) MAX(abs((sq1 >> 3) - (sq2 >> 3)), abs((sq1 & 7) - (sq2 & 7)))

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

#define FLIP(sq) ((sq) ^ 56)

typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
#define C64(constantU64) constantU64##ULL

#define MAX_PLY 64
#define STACK_SIZE 16000
#define MAX_HIST 700
#define MAX_HASH 256

// tt flags
#define HASH_FLAG_EXACT 0
#define HASH_FLAG_ALPHA 1
#define HASH_FLAG_BETA 2
#define HASH_FLAG_EMPTY 3

#define NO_HASH_ENTRY 100000

// tt data structure
typedef struct {
    U64 key;       // hashed key
    int depth;      // current search depth
    int flags;      // type of node as a tt flag - Fail high (beta), Fail low (alpha), or PV
    int score;      // score of the node within a search
    U16 best_move;   // Best move to be stored
} hash_t;

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

typedef struct {
    U16 move;
    int score;
} move_t;

typedef struct {
    move_t moves[STACK_SIZE];
    int count;
} _move_list;

typedef struct {
    char piece;
    char to;
} counter_move;

typedef struct {
    U16 move;
    U16 flags;
    int fifty_clock;
    int material[2];
    U64 hash;
    counter_move last_move;
} hist_t;

typedef struct {
    U16 bestmove;
    int evaluation;
} quickresult_t;

// search
#define VALWINDOW 50
#define NULLMOVE_R 2
#define DELTA 200

// Evaluation
#define MG 0
#define EG 1

#ifndef M24GC_ENUMS
#define M24GC_ENUMS
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
