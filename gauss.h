#ifndef _GAUSS_H_
#define _GAUSS_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#ifdef _MSC_VER
    #include <windows.h>
#else
    # include <sys/time.h>
#endif

typedef enum {
    a1,b1,c1,d1,e1,f1,g1,h1,
    a2,b2,c2,d2,e2,f2,g2,h2,
    a3,b3,c3,d3,e3,f3,g3,h3,
    a4,b4,c4,d4,e4,f4,g4,h4,
    a5,b5,c5,d5,e5,f5,g5,h5,
    a6,b6,c6,d6,e6,f6,g6,h6,
    a7,b7,c7,d7,e7,f7,g7,h7,
    a8,b8,c8,d8,e8,f8,g8,h8,
} enumSquare;

typedef enum {WHITE, BLACK, BOTH} enumSide;

typedef enum {K,P,N,B,R,Q,k,p,n,b,r,q,_} enumPiece;

enum castleFlags {WCK=1, WCQ=2, BCK=4, BCQ=8};


#define MAXPLY 63
#define MAXHIST 800
#define MAXMOVES 10000

#define NOMOVE 0
#define NULLMOVE 0xFFFF

#define INF 60000
#define MATE_SCORE 50000
#define MATE_THRESHOLD 49900

#define MAX(X,Y) ((X)<(Y) ? (Y) : (X))


typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;
#define C64(X) X##ULL

typedef struct {
    U64 mask;
    U64 magic;
} MagicInfo;

typedef struct {
    U32 state;
} xorshift32_state;

typedef struct {
    enumPiece piece;
    enumSquare to;
} counter_move_t;

typedef struct {
    U64 bitboards[12];
    U64 occupancies[3];
    U8 squares[64];
    short ep_square;
    U8 castle_flags;
    enumSide side;
    int rule50;
    U64 hash;
    counter_move_t last_move;
    U64 game_history[MAXHIST];
    int hply;
} board_t;

typedef struct {
    U16 move;
    int score;
} move_t;

typedef struct {
    move_t* msp[MAXPLY];
    U16 pv[MAXPLY][MAXPLY];
    U16 killers[MAXPLY][2];
    counter_move_t counter_moves[12][64];
    U16 bestmove;
    int ply;
    long nodes;
    int sdepth;
    bool follow_pv;
} search_info_t;

typedef struct {
    U8 captured;
    U8 castle_flags;
    short ep_square;
    int rule50;
    U64 hash;
    counter_move_t last_move;
} hist_t;

typedef struct {
    long time[2];
    long inc[2];
    int movestogo;
    int depth;
    long nodes;
    int mate;
    int movetime;
    U8 flags;
    bool stop;
    long starttime;
    long stoptime;
} timeinfo_t;

typedef struct {
    U64 hash;
    int val;
    U16 bestmove;
    U8 depth;
    U8 flags;
} hash_t;

typedef struct {
    U64 hash;
    int evaluation;
    bool valid;
} hash_eval_t;

typedef struct {
    U8 mob;
    U8 trop;
} piece_eval_t;


#endif
