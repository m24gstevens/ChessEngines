#include "bitboard.h"
#include "common.h"

xorshift32_state seed = {0x8AFBCBAAUL};

/* xorshift PRNG for magic bitboards */
U32 xorshift32(xorshift32_state *st) {
    U32 x = st->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return st->state = x;
}

U32 random_xor() {return xorshift32(&seed);}

U64 random_u64() {
    U64 i1, i2, i3, i4;
    i1 = (U64)random_xor() & 0xFFFF; i2 = (U64)random_xor() & 0xFFFF;
    i3 = (U64)random_xor() & 0xFFFF; i4 = (U64)random_xor() & 0xFFFF;
    return i1 | (i2 << 16) | (i3 << 32) | (i4 << 48);
}

U64 random_u64_sparse() {
    return random_u64() & random_u64() & random_u64();
}