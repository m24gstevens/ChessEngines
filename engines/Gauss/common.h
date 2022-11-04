#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;
#define C64(X) X##ULL

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

// PRNG

typedef struct {
    U32 state;
} xorshift32_state;

U64 random_u64_sparse();

#endif