/* KQKGen.c - move generation and ungeneration
When using terms "White" and "Black", we assume that white has the queen and black doesn't */
#include "KQK.h"

void print_bb(U64 bb) {
    int i,j;
    printf("\n");
    for (i=7; i >= 0; i--) {
        for (j=0; j<8; j++)
            printf("%d ", ((bb >> (8*i + j)) & 1) ? 1 : 0);
        printf("\n");
    }
}

static const U64 not_a_file = ~0x0101010101010101;
static const U64 not_h_file = ~0x8080808080808080;
static const U64 main_diag = C64(0x8040201008040201);
static const U64 main_antidiag = C64(0x0102040810204080);

U64 north_one(U64 bb) {return bb << 8;}
U64 south_one(U64 bb) {return bb >> 8;}
U64 east_one(U64 bb) {return (bb << 1) & not_a_file;}
U64 west_one(U64 bb) {return (bb >> 1) & not_h_file;}

U64 rank_mask(char sq) {return C64(0xFF) << (sq & 56);}
U64 file_mask(char sq) {return C64(0x0101010101010101) << (sq & 7);}
U64 diagonal_mask(char sq) {
    int diag = 8*(sq & 7) - (sq & 56);
    int nort = -diag & (diag >> 31);
    int sout = diag & (-diag >> 31);
    return (main_diag >> sout) << nort;
}

U64 antidiagonal_mask(char sq) {
    int diag = 56 - 8*(sq & 7) - (sq & 56);
    int nort = -diag & (diag >> 31);
    int sout = diag & (-diag >> 31);
    return (main_antidiag >> sout) << nort;
}

U64 king_attack_set(char sq) {
    U64 bb = (U64)1 << sq;
    U64 attacks = east_one(bb) | west_one(bb);
    bb |= attacks;
    attacks |= north_one(bb) | south_one(bb);
    return attacks;
}

void init_ray_attacks(U64 attacks[8][64]) {
    char i;
    U64 mask;

    for (i=0; i<64; i++) {
        mask = diagonal_mask(i);
        attacks[0][i] = positive_ray(mask, i);
        attacks[7][i] = negative_ray(mask, i);
        mask = file_mask(i);
        attacks[1][i] = positive_ray(mask, i);
        attacks[6][i] = negative_ray(mask, i);
        mask = antidiagonal_mask(i);
        attacks[2][i] = positive_ray(mask, i);
        attacks[5][i] = negative_ray(mask, i);
        mask =rank_mask(i);
        attacks[3][i] = positive_ray(mask, i);
        attacks[4][i] = negative_ray(mask, i);
    } 
}

int naive_ms1b(int i) {
    int j;
    int r=0;
    for (j=1; j<8; j++) {
        if (i & (1 << j))
            r = j;
    }
    return r;
}

int bitscan_reverse(U64 bb, char table[0xFF]) {
    int result = 0;
    if (bb > 0xFFFFFFFF) {
        bb >>= 32;
        result = 32;
    }
    if (bb > 0xFFFF) {
        bb >>= 16;
        result += 16;
    }
    if (bb > 0xFF) {
        bb >>= 8;
        result += 8;
    }
    return result + table[bb];
}


void gen_queen_attack_tables(U64 ray_attacks[8][64], char ms1bTable[0xFF], U64 table[64][64]) {
    U64 occupied, attack, d_attacks, blocker;
    int i,j, dir;
    char sq;
    for (i=0; i<64; i++) {
        for (j=0; j<64; j++) {
            if (j==i)
                continue;
            occupied = ((U64)1 << j);
            attack = 0; 
            for (dir=0;dir<4;dir++) {
                d_attacks = ray_attacks[dir][i];
                blocker = d_attacks & occupied;
                if (blocker) {
                    sq = BITSCAN(blocker);
                    d_attacks ^= ray_attacks[dir][sq];
                }
                attack |= d_attacks;
            }
            for (dir=4;dir<8;dir++) {
                d_attacks = ray_attacks[dir][i];
                blocker = d_attacks & occupied;
                if (blocker) {
                    sq = bitscan_reverse(blocker, ms1bTable);
                    d_attacks ^= ray_attacks[dir][sq];
                }
                attack |= d_attacks;
            }
            table[i][j] = attack;
        }
    }
}

void gen_white_moves(U16 p, U64 queen_attack_set, pos_list *pl) {
    int i=0;
    char ak = _ak(p);
    char aq = _aq(p);
    char sq;
    while (queen_attack_set) {
        sq = BITSCAN(queen_attack_set);
        p &= ~(0x3F << 6);
        p |= (sq << 6);
        if (is_legal(p))
            pl->positions[i++] = p;
        p &= ~(0x3F << 6);
        p |= (aq << 6);
        queen_attack_set &= (queen_attack_set - 1);
    }

    queen_attack_set = king_attack_set(ak) ^ ((U64)1 << ak);
    while (queen_attack_set) {
        sq = BITSCAN(queen_attack_set);
        p &= ~0x3F;
        p |= sq;
        if (is_legal(p))
            pl->positions[i++] = p;
        p &= ~0x3F;
        p |= ak;
        queen_attack_set &= (queen_attack_set - 1);
    }
    pl->length = i;
}

void gen_black_ancestors(U16 p, pos_list *pl) {
    int i=0;
    char dk = _dk(p);
    char sq;

    U64 attack_set = king_attack_set(dk) ^ ((U64)1 << dk);
    while (attack_set) {
        sq = BITSCAN(attack_set);
        p &= ~(0xF << 12);
        p |= (sq << 12);
        if (is_legal(p))
            pl->positions[i++] = p;
        p &= ~(0xF << 12);
        p |= (dk << 12);
        attack_set &= (attack_set - 1);
    }
    pl->length = i;
}

void gen_white_parents(U16 p, U64 queen_attack_table[64][64], pos_list *pl) {
    gen_white_moves(p,queen_attack_table[_aq(p)][_ak(p)],pl);
    U16 test;
    int i;
    int j=0;
    for (i=0;i < pl->length; i++) {
        test = pl->positions[i];
        if (!is_check(test, queen_attack_table[_aq(test)][_ak(test)] | king_attack_set(_ak(test))))
            pl->positions[j++] = test;
    }
    pl->length = j;
} 

void gen_white_children(U16 p, U64 queen_attack_table[64][64], pos_list *pl) {
    gen_black_ancestors(p, pl);
    U16 test;
    int i;
    int j=0;
    for (i=0;i < pl->length; i++) {
        test = pl->positions[i];
        if (!is_check(test, queen_attack_table[_aq(test)][_ak(test)] | king_attack_set(_ak(test))))
            pl->positions[j++] = test;
    }
    pl->length = j;
}