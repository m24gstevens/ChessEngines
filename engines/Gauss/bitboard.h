#ifndef _BITBOARD_H_
#define _BITBOARD_H_

#include <stdio.h>
#include "common.h"

// Magic bitboards

// Bit macros
#define GET_LS1B(X) X&-X
#define POP_LS1B(X) X&=(X-1)
#define SET_BIT(X,n) X|=(C64(1)<<(n))
#define TEST_BIT(X,n) X&(C64(1)<<(n))

#define northOne(bb) (bb << 8)
#define southOne(bb) (bb >> 8)
#define northTwo(bb) (bb << 16)
#define southTwo(bb) (bb >> 16)
#define eastOne(bb) ((bb & ~C64(0x8080808080808080)) << 1)
#define westOne(bb) ((bb & ~C64(0x0101010101010101)) >> 1)
#define eastTwo(bb) ((bb & ~C64(0xc0c0c0c0c0c0c0c0)) << 2)
#define westTwo(bb) ((bb & ~C64(0x0303030303030303)) >> 2)

// Helpers
void print_bitboard(U64);

// Bit twiddles
int popcount(U64);
int bitscanForward(U64);


// Attack tables
void init_attack_tables();
void generate_magic_numbers();

extern U64 knightAttackTable[64];
extern U64 kingAttackTable[64];
extern U64 pawnAttackTable[2][64];
extern U64 bishopAttackTable[64][512]; 
extern U64 rookAttackTable[64][4096];

extern MagicInfo magicBishopInfo[64];
extern MagicInfo magicRookInfo[64];

extern const int bishopBits[64];
extern const int rookBits[64];

inline U64 bishopAttacks(enumSquare sq, U64 occ) {
    occ &= magicBishopInfo[sq].mask;
    occ *= magicBishopInfo[sq].magic;
    occ >>= (64 - bishopBits[sq]);
    return bishopAttackTable[sq][occ];
}
inline U64 rookAttacks(enumSquare sq, U64 occ) {
    occ &= magicRookInfo[sq].mask;
    occ *= magicRookInfo[sq].magic;
    occ >>= (64 - rookBits[sq]);
    return rookAttackTable[sq][occ];
}

#endif