#include "bitboard.h"
#include "common.h"
#include "tt.h"
#include "order.h"

// Useful positions

char *starting_position = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
char *kiwipete = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ";
char *capture_position = "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1 ";
char *unsafe_king = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";

// Xorshift PRNG

xorshift32_state seed = {0x4828501};

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

// Hashing the position

U64 piece_keys[13][64];
U64 ep_keys[2][8];
U64 castle_keys[16];
U64 side_key;

void init_random_keys() {
    int i,j;
    for (i=0;i<12;i++) {
        for (j=0; j<64; j++) {
            piece_keys[i][j] = random_u64();
        }
    }
    for (j=0;j<64;j++) {
        piece_keys[12][j] = C64(0);
    }
    for (i=0;i<2;i++) {
        for (j=0; j<8; j++) {
            ep_keys[i][j] = random_u64();
        }
    }
    for (i=0;i<16;i++) {
        castle_keys[i] = random_u64();
    }
    side_key = random_u64();
}

U64 hash_position(board_t* board) {
    enumSquare i;
    enumPiece pc;
    U64 bitboard, key;

    key = C64(0);
    for (i=0;i<64;i++) {
        pc = board->squares[i];
        key ^= piece_keys[pc][i];
    }
    if (board->ep_square != -1) {
        key ^= ep_keys[board->side][board->ep_square % 8];
    }
    key ^= castle_keys[board->castle_flags];
    if (board->side) {key ^= side_key;}

    return key;
}

// Timing functions

long get_time_ms() {
    #ifdef _MSC_VER
        return GetTickCount();
    #else
        struct timeval time_value;
        gettimeofday(&time_value, NULL);
        return time_value.tv_sec * 1000 + time_value.tv_usec / 1000;
    #endif
}

// Initialization

void init_tables() {
    init_attack_tables();
    init_random_keys();
    clear_history_heuristic();
    tt_setsize(0x4000000);  //0x4000000
}

// History