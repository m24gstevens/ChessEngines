#include "bitboard.h"
#include "common.h"

/* Helpers */

void print_bitboard(U64 bb) {
    printf("\n");
    for (int r=7;r>=0;r--) {
        printf("%c  ",'1'+r);
        for (int f=0;f<8;f++) {
            printf("%d ",bb&(C64(1)<<(8*r+f)) ? 1 : 0);
        }
        printf("\n");
    }
    printf("\n   A B C D E F G H \n");
}

/* Bit twiddling */

static inline int popcount32(U32 b) {
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
        POP_LS1B(bb);
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

// Index of LS1B
int bitscanForward(U64 bb) {
    const U64 debruijn64 = C64(0x03f79d71b4cb0a89);
    return deBruijnLookup[((bb & -bb) * debruijn64) >> 58];
}

/* Attack Sets */

U64 knightAttackTable[64];
U64 kingAttackTable[64];
U64 pawnAttackTable[2][64];

static inline U64 knight_attacks(U64 bb) {
    U64 temp = eastOne(bb) | westOne(bb);
    U64 attacks = northTwo(temp) | southTwo(temp);
    temp =  eastTwo(bb) | westTwo(bb);
    attacks |= northOne(temp) | southOne(temp);
    return attacks;
}

static inline void init_knight_attacks() {
    for (int i=0; i<64; i++) {
        knightAttackTable[i] = knight_attacks(C64(1)<<i);
    }
}

static inline U64 king_attacks(U64 bb) {
    U64 attacks = bb | eastOne(bb) | westOne(bb);
    attacks |= northOne(attacks) | southOne(attacks);
    return attacks ^ bb;
}

static inline void init_king_attacks() {
    for (int i=0;i<64;i++) {
        kingAttackTable[i] = king_attacks(C64(1)<<i);
    }
}

static inline U64 pawn_attacks(U64 bb, enumSide s) {
    U64 aside = westOne(bb) | eastOne(bb);
    return s ? southOne(aside) : northOne(aside); 
}

static inline void init_pawn_attacks() {
    for (int i=0; i<64; i++) {
        pawnAttackTable[WHITE][i] = pawn_attacks(C64(1)<<i,WHITE);
        pawnAttackTable[BLACK][i] = pawn_attacks(C64(1)<<i,BLACK);
    }
}

/* Magic bitboards */

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

const U64 rookMagic[64] = {
  C64(0xa800080c0001020),
  C64(0x1480200080400010),
  C64(0x9100200008104101),
  C64(0x100041000082100),
  C64(0x2200020010200804),
  C64(0x200010804100200),
  C64(0x41000100020000c4),
  C64(0x300160040822300),
  C64(0x2800280604000),
  C64(0x80c00040201000),
  C64(0x40802000801000),
  C64(0x2004008220010),
  C64(0x402800400080082),
  C64(0x806000200100408),
  C64(0x2004001102241038),
  C64(0x8641000082004100),
  C64(0x288004400880),
  C64(0x8106848040022000),
  C64(0x1060008010002080),
  C64(0x1001010020081000),
  C64(0x8010004891100),
  C64(0x6808002008400),
  C64(0x4040008114290),
  C64(0x20004006081),
  C64(0x400080208008),
  C64(0x1280400040201000),
  C64(0x3020200180100080),
  C64(0x410080080100084),
  C64(0x50040080080080),
  C64(0x4020080800400),
  C64(0x4001006100020024),
  C64(0x200140200019a51),
  C64(0x88804010800420),
  C64(0x8810004000c02000),
  C64(0x8a008022004013),
  C64(0x4008008208801000),
  C64(0x4008002004040040),
  C64(0xd244800200800400),
  C64(0x12a100114005882),
  C64(0x8008044020000a1),
  C64(0x8840058004458020),
  C64(0x90002000d0014000),
  C64(0x420001008004040),
  C64(0x8080080010008080),
  C64(0x1006001008220004),
  C64(0x408841020080140),
  C64(0x10080210040001),
  C64(0x8002040060820001),
  C64(0x40810a002600),
  C64(0x342a0100844200),
  C64(0x840110020084100),
  C64(0x4208001000088080),
  C64(0x10c041008010100),
  C64(0x82001024085200),
  C64(0x800800100020080),
  C64(0x2000010850840200),
  C64(0x80002018408301),
  C64(0x82402210820506),
  C64(0x82200801042),
  C64(0x9220042008100101),
  C64(0x342000820041102),
  C64(0x402008104502802),
  C64(0x441001200088421),
  C64(0x42401288c090042),
};

const U64 bishopMagic[64] = {
  C64(0x840a20802008011),
  C64(0x802100400908001),
  C64(0x12141042008249),
  C64(0x8204848004882),
  C64(0xd00510c004808246),
  C64(0x4022020320000010),
  C64(0x4050880882100800),
  C64(0x4802402208200401),
  C64(0x100204810808090),
  C64(0xc400100242004200),
  C64(0x1820c20400408000),
  C64(0x282080202000),
  C64(0x20210000c00),
  C64(0x2001488824401020),
  C64(0x209008401201004),
  C64(0x20010846322012),
  C64(0xc040111090018100),
  C64(0x4001004a09406),
  C64(0x8004405001020014),
  C64(0x8001c12102020),
  C64(0x1001000290400200),
  C64(0x841001820884000),
  C64(0x88c010124010440),
  C64(0x410a0000211c0200),
  C64(0x4c04402010020820),
  C64(0x2288110010800),
  C64(0x2084040080a0040),
  C64(0x30020020080480e0),
  C64(0x84040004410040),
  C64(0xc000888001082000),
  C64(0x4088208048a1004),
  C64(0x14004014222200),
  C64(0x30a200410305060),
  C64(0x804010806208200),
  C64(0x82002400120800),
  C64(0x200020082180081),
  C64(0x8240090100001040),
  C64(0xd200a100420041),
  C64(0x9c50040048010140),
  C64(0x8002004a0b084200),
  C64(0x8220a0105140),
  C64(0x14210442001030),
  C64(0x2140124000800),
  C64(0x1008102018000908),
  C64(0x48d0a000400),
  C64(0x1013003020081),
  C64(0x2020204183210),
  C64(0x2020204220200),
  C64(0x1080202620002),
  C64(0x2202609103800c1),
  C64(0x50183020841040a0),
  C64(0x800108246080020),
  C64(0x10084010248000),
  C64(0x40942480200c2),
  C64(0x4200842008812),
  C64(0x10010824888800),
  C64(0x1010080a00800),
  C64(0x608008400825000),
  C64(0x8021540040441000),
  C64(0x2022484800840400),
  C64(0x100000040810b400),
  C64(0x90044500a0208),
  C64(0x94010520a0041),
  C64(0x40110141030100),
};

MagicInfo magicBishopInfo[64];
MagicInfo magicRookInfo[64];

U64 bishopAttackTable[64][512]; 
U64 rookAttackTable[64][4096];

/* Rook and bishop attack mask and occupancy sets */
static inline U64 rook_attack_mask(enumSquare sq, U64 block) {
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

static inline U64 rook_occupancy_mask(enumSquare sq) {
    int rk = (sq >> 3);
    int fl = (sq & 7);
    U64 occ = C64(0);
    for (int r=rk+1; r <= 6; r++) occ |= ((U64)1 << (fl + 8*r));
    for (int r=rk-1; r >= 1; r--) occ |= ((U64)1 << (fl + 8*r));
    for (int f=fl+1; f <= 6; f++) occ |= ((U64)1 << (f + 8*rk));
    for (int f=fl-1; f >= 1; f--) occ |= ((U64)1 << (f + 8*rk));
    return occ;
}

static inline U64 bishop_attack_mask(enumSquare sq, U64 block) {
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

static inline U64 bishop_occupancy_mask(enumSquare sq) {
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


static inline int magic_transform(U64 occupancy, U64 magic, int bits) {
    return (int)((occupancy * magic) >> (64 - bits));
}

static inline U64 index_to_u64(int idx, int num_bits, U64 mask) {
    U64 occupancy = (U64)0;
    for (int count=0; count < num_bits; count++) {
        int lsb = bitscanForward(mask);
        POP_LS1B(mask);
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

static inline void init_rook_magics() {
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

static inline void init_bishop_magics() {
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

U64 kingAttacks(enumSquare sq) {
    return kingAttackTable[sq];
}

U64 knightAttacks(enumSquare sq) {
    return knightAttackTable[sq];
}

U64 pawnAttacks(enumSide side, enumSquare sq) {
    return pawnAttackTable[side][sq];
}

/* inline U64 bishopAttacks(enumSquare sq, U64 occ) {
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
} */

void init_attack_tables() {
    init_knight_attacks();
    init_king_attacks();
    init_pawn_attacks();
    init_bishop_magics();
    init_rook_magics();
}




