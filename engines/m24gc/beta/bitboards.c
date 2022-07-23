#include "defs.h"
#include "data.h"
#include "protos.h"

xorshift32_state seed = {0x8AFBCBAAUL};

MagicInfo magicBishopInfo[64];
MagicInfo magicRookInfo[64];

const U64 aFile = C64(0x0101010101010101);
const U64 hFile = C64(0x8080808080808080);
const U64 mainDiag = C64(0x8040201008040201);
const U64 mainAntiDiag = C64(0x0102040810204080);

// Number of bits for magic bitboards
const int rookBits[64] = {
    12,11,11,11,11,11,11,12,
    11,10,10,10,10,10,10,11,
    11,10,10,10,10,10,10,11,
    11,10,10,10,10,10,10,11,
    11,10,10,10,10,10,10,11,
    11,10,10,10,10,10,10,11,
    11,10,10,10,10,10,10,11,
    12,11,11,11,11,11,11,12
};
const int bishopBits[64] = {
    6,5,5,5,5,5,5,6,
    5,5,5,5,5,5,5,5,
    5,5,7,7,7,7,5,5,
    5,5,7,9,9,7,5,5,
    5,5,7,9,9,7,5,5,
    5,5,7,7,7,7,5,5,
    5,5,5,5,5,5,5,5,
    6,5,5,5,5,5,5,6
};

// Magic numbers for magic bitboards
const U64 rookMagic[64] = {
  C64(0x280004002a11080),
  C64(0x80200040008010),
  C64(0x4080200008100080),
  C64(0x100040821001000),
  C64(0x500100248010004),
  C64(0xc100040001000208),
  C64(0x80020000800100),
  C64(0x100004200902100),
  C64(0x800038804002),
  C64(0x81402010004000),
  C64(0x801100200010410a),
  C64(0x2a02800802100080),
  C64(0x891001100080104),
  C64(0x82000805020010),
  C64(0x4402000200010804),
  C64(0xa014800080004100),
  C64(0x410888000400030),
  C64(0x60008040002080),
  C64(0x20820040102203),
  C64(0x8010008010800800),
  C64(0x1088004004004201),
  C64(0x4000808002000400),
  C64(0x240040002010810),
  C64(0x1041020000810044),
  C64(0x800100204100),
  C64(0x420100040004026),
  C64(0x210021080200281),
  C64(0x20080180500080),
  C64(0x1004040080080280),
  C64(0x1200040080800200),
  C64(0x1a050a1400082530),
  C64(0x1000010200004084),
  C64(0x420400028800081),
  C64(0x440011041002080),
  C64(0x812c110041002000),
  C64(0x401100081801802),
  C64(0x80800402800800),
  C64(0x50a000280800400),
  C64(0x10a104000208),
  C64(0x342408042000104),
  C64(0xc0003440808000),
  C64(0x4d00400100810020),
  C64(0xa001440820021),
  C64(0x1280900100690020),
  C64(0x6000080004008080),
  C64(0x5001000400890002),
  C64(0xc020001008080),
  C64(0x410108049020024),
  C64(0x240820021004200),
  C64(0x8810028400100),
  C64(0x2209004020001100),
  C64(0xc00012a108420200),
  C64(0x210040008008080),
  C64(0x8030020004008080),
  C64(0x2140704108020400),
  C64(0x1008400410200),
  C64(0x8a21100804602),
  C64(0x2017c000a0110085),
  C64(0x2048c050200501),
  C64(0x2202002008400412),
  C64(0x6180118500480015),
  C64(0x41a5000804000201),
  C64(0x3804009041020804),
  C64(0x1000082003041)
};

const U64 bishopMagic[64] = {
  C64(0x4100408008010),
  C64(0x8928082060150),
  C64(0x44440082042014),
  C64(0x31182a0020000210),
  C64(0x62020210000100a0),
  C64(0x5002029004100020),
  C64(0x4a4012410440040),
  C64(0x442401201004),
  C64(0x802048d18082101),
  C64(0x806208409021220),
  C64(0x20100082005900),
  C64(0x8404a102048200),
  C64(0x8000cd10c0000040),
  C64(0x36000208020840c0),
  C64(0x1440128821080),
  C64(0x8000120520821040),
  C64(0xe44002220040123),
  C64(0x6048401001210420),
  C64(0x5001003001042100),
  C64(0x84081802400800),
  C64(0x2c000888a00080),
  C64(0x4808e00110101000),
  C64(0x1021008051082000),
  C64(0x4000201441080800),
  C64(0x43080410082008a0),
  C64(0x24042100020a0c00),
  C64(0x26010102b0141020),
  C64(0x8708080030820002),
  C64(0x81c040010410042),
  C64(0x490004900800),
  C64(0x2008020005009220),
  C64(0x20011020c2088400),
  C64(0x1200980200800),
  C64(0x905000440403),
  C64(0x105000080281),
  C64(0x4000e00800810810),
  C64(0x240010040c40404),
  C64(0xb001000080a0),
  C64(0x1001604802020a),
  C64(0x12020041442420),
  C64(0x11011402a0404100),
  C64(0x100d082202009001),
  C64(0x108822801000800),
  C64(0x884200808810),
  C64(0x4020204106200),
  C64(0x8c4811001000608),
  C64(0x4084805040040),
  C64(0x1010020200240040),
  C64(0x4810848820320340),
  C64(0x824410346185),
  C64(0x30c0002908480008),
  C64(0x40090288840401c4),
  C64(0x14002004240103),
  C64(0x10010a0410102a3),
  C64(0x1044081808188100),
  C64(0x308010c008200),
  C64(0x20a9808808122202),
  C64(0x8420008145303004),
  C64(0x2000000200841c11),
  C64(0xc000000003040910),
  C64(0x51504412020211),
  C64(0x8024420141102),
  C64(0x8008200210026080),
  C64(0xa9020084050202)
};

/* Utilities */
void print_bitboard(U64 bb) {
    printf("\n");
    for (int i=7; i>=0; i--) {
        printf("%d   ", i+1);
        for (int j=0; j<8;j++) {
            printf("%d ", ((C64(1) << (8*i + j)) & bb) ? 1 : 0);
        }
        printf("\n");
    }
    printf("\n    ");
    for (int j=0; j<8; j++)
        printf("%c ", 'a' + j);
}

/* Classical bit-twiddling */
int popcount32(U32 b) {
    int c;
    b = b - ((b >> 1) & 0x55555555UL);
    b = (b & 0x33333333UL) + ((b >> 2) & 0x33333333UL);
    c = ((b + (b >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
    return c;
}

int popcount(U64 bb) {
    return popcount32(bb & 0xFFFFFFFFUL) + popcount32(bb >> 32);
}

int popcount_sparse(U64 bb) {
    int result=0;

    while(bb) {
        clear_ls1b(bb);
        result++;
    }
    return result;
}

static const int deBruijnLookup[64] = {
    0,1,48,2,57,49,28,3,
    61,58,50,42,38,29,17,4,
    62,55,59,36,53,51,43,22,
    45,39,33,30,24,18,12,5,
    63,47,56,27,60,41,37,16,
    54,35,52,21,44,32,23,11,
    46,26,40,15,34,20,31,10,
    25,14,19,9,13,8,7,6
};

/* Get index of least significant 1 bit */
int bitscanForward(U64 bb) {
    const U64 debruijn64 = C64(0x03f79d71b4cb0a89);
    return deBruijnLookup[((bb & -bb) * debruijn64) >> 58];
}

/* generating attack masks */
U64 pawn_attack_mask(int side, int idx) {
    U64 pawn_squares = (U64)1 << idx;
    U64 attacks = C64(0);

    if (!side) {
        attacks |= northOne(westOne(pawn_squares));
        attacks |= northOne(eastOne(pawn_squares));
    } else {
        attacks |= southOne(westOne(pawn_squares));
        attacks |= southOne(eastOne(pawn_squares));
    }
    return attacks;
}

U64 knight_attack_mask(int idx) {
    U64 temp = eastOne((U64)1 << idx) | westOne((U64)1 << idx);
    U64 attacks = northTwo(temp) | southTwo(temp);
    temp =  eastTwo((U64)1 << idx) | westTwo((U64)1 << idx);
    attacks |= northOne(temp) | southOne(temp);
    return attacks;
}

U64 king_attack_mask(int idx) {
    U64 attacks = eastOne((U64)1 << idx) | westOne((U64)1 << idx);
    attacks |= (northOne(attacks) | southOne(attacks));
    attacks |= northOne((U64)1 << idx) | southOne((U64)1 << idx);
    return attacks;
}

void init_jumper_attack_masks() {
    for (int i=0; i<64; i++) {
        pawn_attack_table[WHITE][i] = pawn_attack_mask(WHITE, i);
        pawn_attack_table[BLACK][i] = pawn_attack_mask(BLACK, i);
        king_attack_table[i] = king_attack_mask(i);
        knight_attack_table[i] = knight_attack_mask(i);
    }
}

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

/* Rook and bishop attack mask and occupancy sets */
U64 rook_attack_mask(int sq, U64 block) {
    int rk = (sq >> 3);
    int fl = (sq & 7);
    int r,f;
    U64 attack = C64(0);
    for (r=rk+1; r <= 7; r++) {
        attack |= ((U64)1 << (fl + 8*r));
        if (block & ((U64)1 << (fl + 8*r))) break;
    }
    for (r=rk-1; r >= 0; r--) {
        attack |= ((U64)1 << (fl + 8*r));
        if (block & ((U64)1 << (fl + 8*r))) break;
    }
    for (f=fl+1; f <= 7; f++) {
        attack |= ((U64)1 << (f + 8*rk));
        if (block & ((U64)1 << (f + 8*rk))) break;
    }
    for (f=fl-1; f >= 0; f--) {
        attack |= ((U64)1 << (f + 8*rk));
        if (block & ((U64)1 << (f + 8*rk))) break;
    }
    return attack;
}

U64 rook_occupancy_mask(int sq) {
    int rk = (sq >> 3);
    int fl = (sq & 7);
    U64 occ = C64(0);
    for (int r=rk+1; r <= 6; r++) occ |= ((U64)1 << (fl + 8*r));
    for (int r=rk-1; r >= 1; r--) occ |= ((U64)1 << (fl + 8*r));
    for (int f=fl+1; f <= 6; f++) occ |= ((U64)1 << (f + 8*rk));
    for (int f=fl-1; f >= 1; f--) occ |= ((U64)1 << (f + 8*rk));
    return occ;
}

U64 bishop_attack_mask(int sq, U64 block) {
    int rk = (sq >> 3);
    int fl = (sq & 7);
    int r,f;
    U64 attack = C64(0);
    for (r=rk+1, f=fl+1; r <= 7 && f <= 7; r++, f++) {
        attack |= ((U64)1 << (f + 8*r));
        if (block & ((U64)1 << (f + 8*r))) break;
    }
    for (r=rk+1, f=fl-1; r <= 7 && f >= 0; r++, f--) {
        attack |= ((U64)1 << (f + 8*r));
        if (block & ((U64)1 << (f + 8*r))) break;
    }
    for (r=rk-1, f=fl+1; r >= 0 && f <= 7; r--, f++) {
        attack |= ((U64)1 << (f + 8*r));
        if (block & ((U64)1 << (f + 8*r))) break;
    }
    for (r=rk-1, f=fl-1; r >= 0 && f >= 0; r--, f--) {
        attack |= ((U64)1 << (f + 8*r));
        if (block & ((U64)1 << (f + 8*r))) break;
    }
    return attack;
}

U64 bishop_occupancy_mask(int sq) {
    int rk = (sq >> 3);
    int fl = (sq & 7);
    int r,f;
    U64 occ = C64(0);
    for (r=rk+1, f=fl+1; r <= 6 && f <= 6; r++, f++) occ |= ((U64)1 << (f + 8*r));
    for (r=rk+1, f=fl-1; r <= 6 && f >= 1; r++, f--) occ |= ((U64)1 << (f + 8*r));
    for (r=rk-1, f=fl+1; r >= 1 && f <= 6; r--, f++) occ |= ((U64)1 << (f + 8*r));
    for (r=rk-1, f=fl-1; r >= 1 && f >= 1; r--, f--) occ |= ((U64)1 << (f + 8*r));
    return occ;
}


int magic_transform(U64 occupancy, U64 magic, int bits) {
    return (int)((occupancy * magic) >> (64 - bits));
}

U64 index_to_u64(int idx, int num_bits, U64 mask) {
    U64 occupancy = (U64)0;
    for (int count=0; count < num_bits; count++) {
        int lsb = bitscanForward(mask);
        clear_ls1b(mask);
        if (idx & (1 << count)) {
            occupancy |= (1ULL << lsb);
        }
    }
    return occupancy;
}

U64 get_magic_bishop_mask(int sq, int m) {
    U64 attacks[4096], occupancies[4096], used[4096], mask, magic;
    int i,j,n;
    mask = bishop_occupancy_mask(sq);
    for (i=0;i<(1 << m); i++) {
        occupancies[i] = index_to_u64(i, m, mask);
        attacks[i] = bishop_attack_mask(sq, occupancies[i]);
    }
    while(1) {
        magic = random_u64_sparse();
        if (popcount(((mask * magic) & C64(0xFF00000000000000))) < 6) continue;
        memset(used, 0, sizeof(used));
        for (i=0; i<(1 << m); i++) {
            j = magic_transform(occupancies[i], magic, m);
            if (used[j] == C64(0)) used[j] = attacks[i];
            else if (used[j] != attacks[i]) goto next;
        }
    return magic;
    next:
        continue;
    }
}

U64 get_magic_rook_mask(int sq, int m) {
    U64 attacks[4096], occupancies[4096], used[4096], mask, magic;
    int i,j,n;
    mask = rook_occupancy_mask(sq);
    for (i=0;i<(1 << m); i++) {
        occupancies[i] = index_to_u64(i, m, mask);
        attacks[i] = rook_attack_mask(sq, occupancies[i]);
    }
    while(1) {
        magic = random_u64_sparse();
        if (popcount(((mask * magic) & C64(0xFF00000000000000))) < 6) continue;
        memset(used, 0, sizeof(used));
        for (i=0; i<(1 << m); i++) {
            j = magic_transform(occupancies[i], magic, m);
            if (used[j] == C64(0)) used[j] = attacks[i];
            else if (used[j] != attacks[i]) goto next;
        }
    return magic;
    next:
        continue;
    }
}

void generate_magic_numbers() {
    int square;
    printf("const U64 rookMagic[64] = {\n");
    for (square=0;square<64;square++)
        printf("  C64(0x%llx),\n", get_magic_rook_mask(square, rookBits[square]));
    printf("};\n\n");
    printf("const U64 bishopMagic[64] = {\n");
    for (square=0;square<64;square++)
        printf("  C64(0x%llx),\n", get_magic_bishop_mask(square, bishopBits[square]));
    printf("};\n\n");
}



/* slider attack tables */

void init_rook_magics() {
    int i,j,n;
    U64 occupancy, occ, mask;
    for (i=0;i<64;i++) {
        MagicInfo info = {.mask=rook_occupancy_mask(i), .magic=rookMagic[i]};
        magicRookInfo[i] = info;
    }
    for (i=0;i<64;i++) {
        n = rookBits[i];
        occupancy = rook_occupancy_mask(i);
        for (j=0;j<(1 << n);j++) {
            occ = index_to_u64(j,n,occupancy);
            mask = occ * rookMagic[i];
            mask >>= (64 - n);
            rookAttackTable[i][mask] = rook_attack_mask(i,occ);
        }
    }
}

void init_bishop_magics() {
    int i,j,n;
    U64 occupancy, occ, mask;
    for (i=0;i<64;i++) {
        MagicInfo info = {.mask=bishop_occupancy_mask(i), .magic=bishopMagic[i]};
        magicBishopInfo[i] = info;
    }
    for (i=0;i<64;i++) {
        n = bishopBits[i];
        occupancy = bishop_occupancy_mask(i);
        for (j=0;j<(1 << n);j++) {
            occ = index_to_u64(j,n,occupancy);
            mask = occ * bishopMagic[i];
            mask >>= (64 - n);
            bishopAttackTable[i][mask] = bishop_attack_mask(i,occ);
        }
    }
}

/* getting attack masks */

U64 kingAttacks(int sq) {
    return king_attack_table[sq];
}

U64 knightAttacks(int sq) {
    return knight_attack_table[sq];
}

U64 pawnAttacks(int side, int sq) {
    return pawn_attack_table[side][sq];
}

U64 bishopAttacks(int sq, U64 occ) {
    occ &= magicBishopInfo[sq].mask;
    occ *= magicBishopInfo[sq].magic;
    occ >>= (64 - bishopBits[sq]);
    return bishopAttackTable[sq][occ];
}
U64 rookAttacks(int sq, U64 occ) {
    occ &= magicRookInfo[sq].mask;
    occ *= magicRookInfo[sq].magic;
    occ >>= (64 - rookBits[sq]);
    return rookAttackTable[sq][occ];
}

U64 queenAttacks(int sq, U64 occ) {
    U64 result, tmp;
    tmp=occ;
    occ &= magicBishopInfo[sq].mask;
    occ *= magicBishopInfo[sq].magic;
    occ >>= (64 - bishopBits[sq]);
    result = bishopAttackTable[sq][occ];

    tmp &= magicRookInfo[sq].mask;
    tmp *= magicRookInfo[sq].magic;
    tmp >>= (64 - rookBits[sq]);
    result |= rookAttackTable[sq][tmp];
    return result;
}

// Queen attacks for legal move generation 
U64 queen_attacks_on_the_fly(int sq) {
    return bishop_attack_mask(sq, 0) | rook_attack_mask(sq, 0);
}

void init_queen_attack_table() {
    for (int i=0; i<64; i++) {
        queen_attack_table[i] = queen_attacks_on_the_fly(i);
    }
}


// Like an attack mask but has to "break through" the first blocker
U64 bishop_pin_occupancy_on_the_fly(int sq, U64 block) {
    int rk = (sq >> 3);
    int fl = (sq & 7);
    int r,f,hit;
    U64 attack = C64(0);
    for (hit=0,r=rk+1, f=fl+1; r <= 7 && f <= 7; r++, f++) {
        if (hit)
            attack |= ((U64)1 << (f + 8*r));
        if (block & ((U64)1 << (f + 8*r))) {
            if (hit)
                break;
            hit++;
        }
    }
    for (hit=0,r=rk+1, f=fl-1; r <= 7 && f >= 0; r++, f--) {
        if (hit)
            attack |= ((U64)1 << (f + 8*r));
        if (block & ((U64)1 << (f + 8*r))) {
            if (hit)
                break;
            hit++;
        }
    }
    for (hit=0,r=rk-1, f=fl+1; r >= 0 && f <= 7; r--, f++) {
        if (hit)
            attack |= ((U64)1 << (f + 8*r));
        if (block & ((U64)1 << (f + 8*r))) {
            if (hit)
                break;
            hit++;
        }
    }
    for (hit=0,r=rk-1, f=fl-1; r >= 0 && f >= 0; r--, f--) {
        if (hit)
            attack |= ((U64)1 << (f + 8*r));
        if (block & ((U64)1 << (f + 8*r))) {
            if (hit)
                break;
            hit++;
        }
    }
    return attack;
}

U64 rook_pin_occupancy_on_the_fly(int sq, U64 block) {
    int rk = (sq >> 3);
    int fl = (sq & 7);
    int r,f, hit;
    U64 attack = C64(0);
    for (hit=0,r=rk+1; r <= 7; r++) {
        if (hit)
            attack |= ((U64)1 << (fl + 8*r));
        if (block & ((U64)1 << (fl + 8*r))) {
            if (hit)
                break;
            hit++;
        }
    }
    for (hit=0,r=rk-1; r >= 0; r--) {
        if (hit)
            attack |= ((U64)1 << (fl + 8*r));
        if (block & ((U64)1 << (fl + 8*r))) {
            if (hit)
                break;
            hit++;
        }
    }
    for (hit=0,f=fl+1; f <= 7; f++) {
        if (hit)
            attack |= ((U64)1 << (f + 8*rk));
        if (block & ((U64)1 << (f + 8*rk))) {
            if (hit)
                break;
            hit++;
        }
    }
    for (hit=0,f=fl-1; f >= 0; f--) {
        if (hit)
            attack |= ((U64)1 << (f + 8*rk));
        if (block & ((U64)1 << (f + 8*rk))) {
            if (hit)
                break;
            hit++;
        }
    }
    return attack;
}

void init_bishop_pin_magics() {
    int i,j,n;
    U64 occupancy, occ, mask;
    for (i=0;i<64;i++) {
        MagicInfo info = {.mask=bishop_occupancy_mask(i), .magic=bishopMagic[i]};
        magicBishopInfo[i] = info;
    }
    for (i=0;i<64;i++) {
        n = bishopBits[i];
        occupancy = bishop_occupancy_mask(i);
        for (j=0;j<(1 << n);j++) {
            occ = index_to_u64(j,n,occupancy);
            mask = occ * bishopMagic[i];
            mask >>= (64 - n);
            bishopPinTable[i][mask] = bishop_pin_occupancy_on_the_fly(i,occ);
        }
    }
}

void init_rook_pin_magics() {
    int i,j,n;
    U64 occupancy, occ, mask;
    for (i=0;i<64;i++) {
        MagicInfo info = {.mask=rook_occupancy_mask(i), .magic=rookMagic[i]};
        magicRookInfo[i] = info;
    }
    for (i=0;i<64;i++) {
        n = rookBits[i];
        occupancy = rook_occupancy_mask(i);
        for (j=0;j<(1 << n);j++) {
            occ = index_to_u64(j,n,occupancy);
            mask = occ * rookMagic[i];
            mask >>= (64 - n);
            rookPinTable[i][mask] = rook_pin_occupancy_on_the_fly(i,occ);
        }
    }
}

U64 bishopPinAttacks(int sq, U64 occ) {
    occ &= magicBishopInfo[sq].mask;
    occ *= magicBishopInfo[sq].magic;
    occ >>= (64 - bishopBits[sq]);
    return bishopPinTable[sq][occ];
}
U64 rookPinAttacks(int sq, U64 occ) {
    occ &= magicRookInfo[sq].mask;
    occ *= magicRookInfo[sq].magic;
    occ >>= (64 - rookBits[sq]);
    return rookPinTable[sq][occ];
}

void init_slider_attack_masks() {
    init_bishop_magics();
    init_rook_magics();
    init_queen_attack_table();
    init_bishop_pin_magics();
    init_rook_pin_magics();
}