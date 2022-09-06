#include "defs.h"
#include "data.h"
#include "protos.h"

// Used the famous PeSTO's evaluation function

// Piece values
int piece_phase_values[2][6] = {{20000, 82, 337, 365, 447, 1025}, {20000, 94, 281, 297, 512, 936}};

// Penalties and bonuses for positional factors
int DOUBLED_PAWN_PENALTY = 10;
int ISOLATED_PAWN_PENALTY = 20;
int BACKWARDS_PAWN_PENALTY = 10;
int PASSED_PAWN_BONUS = 25;
int ROOK_SEVENTH_BONUS = 30;
int ROOK_OPEN_FILE_BONUS = 20;
int ROOK_SEMI_OPEN_FILE_BONUS = 12;
int UNDEVELOPED_PIECE_PENALTY = 7;
int UNSTOPPABLE_PASSER = 800;

// Penalties and bonuses for positional factors
int doubled_pawn_penalty[2] = {10,20};
int isolated_pawn_penalty[2] = {12,8};
int backward_pawn_penalty[2] = {15,10};
// [phase][side][rank]
int passed_pawn_bonus[2][2][8] = {{{0, 10, 15, 20, 30, 40, 50, 60},{60, 50, 40, 30, 20, 15, 10, 0}},
{{0, 55, 65, 70, 75, 80, 85, 90},{90, 85, 80, 75, 70, 65, 55, 0}}};
int rook_seventh_bonus[2] = {25, 15};
int rook_open_file_bonus[2] = {25, 15};
int rook_semi_open_bonus[2] = {12, 16};
int NO_PAWNS_PENALTY = 100;

// Piece-Square tables for positional evaluation
// pstables[phase][piece] where the tables are from Black's perspective
int pstables[2][6][64] = {
{{-65,  23,  16, -15, -56, -34,   2,  13,           // [mg][black king][sq]
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14},
    {0,   0,   0,   0,   0,   0,  0,   0,           // [mg][black pawn][sq]
     98, 134,  61,  95,  68, 126, 34, -11,
     -6,   7,  26,  31,  65,  56, 25, -20,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
      0,   0,   0,   0,   0,   0,  0,   0},
    {-167, -89, -34, -49,  61, -97, -15, -107,      //[mg][black knight][sq]
     -73, -41,  72,  36,  23,  62,   7,  -17,
     -47,  60,  37,  65,  84, 129,  73,   44,
      -9,  17,  19,  53,  37,  69,  18,   22,
     -13,   4,  16,  13,  28,  19,  21,   -8,
     -23,  -9,  12,  10,  19,  17,  25,  -16,
     -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23},
    {-29,   4, -82, -37, -25, -42,   7,  -8,        //[mg][black bishop][sq]
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21},
    {32,  42,  32,  51, 63,  9,  31,  43,       //[mg][black rook][sq]
     27,  32,  58,  62, 80, 67,  26,  44,
     -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26},     //[mg][black queen][sq]
    {-28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
     -1, -18,  -9,  10, -15, -25, -31, -50}
    },
{ { -74, -35, -18, -18, -11,  15,   4, -17,     //[eg][black king][sq] etc...
    -12,  17,  14,  17,  17,  38,  23,  11,
     10,  17,  23,  15,  20,  45,  44,  13,
     -8,  22,  24,  27,  26,  33,  26,   3,
    -18,  -4,  21,  24,  27,  23,   9, -11,
    -19,  -3,  11,  21,  23,  16,   7,  -9,
    -27, -11,   4,  13,  14,   4,  -5, -17,
    -53, -34, -21, -11, -28, -14, -24, -43},
    {0,   0,   0,   0,   0,   0,   0,   0,
    178, 173, 158, 134, 147, 132, 165, 187,
     94, 100,  85,  67,  56,  53,  82,  84,
     32,  24,  13,   5,  -2,   4,  17,  17,
     13,   9,  -3,  -7,  -7,  -8,   3,  -1,
      4,   7,  -6,   1,   0,  -5,  -1,  -8,
     13,   8,   8,  10,  13,   0,   2,  -7,
      0,   0,   0,   0,   0,   0,   0,   0},
    {-58, -38, -13, -28, -31, -27, -63, -99,
    -25,  -8, -25,  -2,  -9, -25, -24, -52,
    -24, -20,  10,   9,  -1,  -9, -19, -41,
    -17,   3,  22,  22,  22,  11,   8, -18,
    -18,  -6,  16,  25,  16,  17,   4, -18,
    -23,  -3,  -1,  15,  10,  -3, -20, -22,
    -42, -20, -10,  -5,  -2, -20, -23, -44,
    -29, -51, -23, -15, -22, -18, -50, -64},
    {-14, -21, -11,  -8, -7,  -9, -17, -24,
     -8,  -4,   7, -12, -3, -13,  -4, -14,
      2,  -8,   0,  -1, -2,   6,   0,   4,
     -3,   9,  12,   9, 14,  10,   3,   2,
     -6,   3,  13,  19,  7,  10,  -3,  -9,
    -12,  -3,   8,  10, 13,   3,  -7, -15,
    -14, -18,  -7,  -1,  4,  -9, -15, -27,
    -23,  -9, -23,  -5, -9, -16,  -5, -17},
    {13, 10, 18, 15, 12,  12,   8,   5,
    11, 13, 13, 11, -3,   3,   8,   3,
     7,  7,  7,  5,  4,  -3,  -5,  -3,
     4,  3, 13,  1,  2,   1,  -1,   2,
     3,  5,  8,  4, -5,  -6,  -8, -11,
    -4,  0, -5, -1, -7, -12,  -8, -16,
    -6, -6,  0,  2, -9,  -9, -11,  -3,
    -9,  2,  3, -1, -5, -13,   4, -20},
  {-9,  22,  22,  27,  27,  19,  10,  20,
    -17,  20,  32,  41,  58,  25,  30,   0,
    -20,   6,   9,  49,  47,  35,  19,   9,
      3,  22,  24,  45,  57,  40,  57,  36,
    -18,  28,  19,  47,  31,  34,  39,  23,
    -16, -27,  15,   6,   9,  17,  10,   5,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -33, -28, -22, -43,  -5, -32, -20, -41}
} 
};

int passed_pawns_bonuses[8] = {0, 10, 20, 30, 40, 50, 60, 70};

// Constants for evaluation
static const U64 firstRank = C64(0xFF);
static const U64 lastRank = C64(0xFF00000000000000);
static const U64 secondRank = C64(0xFF00);
static const U64 seventhRank = C64(0xFF000000000000);
static const U64 queenside = C64(0x0707070707070707);
static const U64 kingside = C64(0xE0E0E0E0E0E0E0E0);
static const U64 centre = C64(0x1818181818181818);
static const U64 white_half = C64(0xFFFFFFFF);
static const U64 secondSeventh = C64(0xFF00000000FF00);
static const U64 thirdSixth = C64(0xFF0000FF0000);
static const U64 fourthFifth = C64(0xFFFF000000);

// Bitboard routines for evaluation
static inline U64 northFill(U64 bb) {
    bb |= (bb << 8);
    bb |= (bb << 16);
    bb |= (bb << 32);
    return bb;
}
static inline U64 southFill(U64 bb) {
    bb |= (bb >> 8);
    bb |= (bb >> 16);
    bb |= (bb >> 32);
    return bb;
}
U64 fileFill(U64 bb) {
    return northFill(bb) | southFill(bb);
}

// Tables for the evaluation
// frontSpan[side][square]
U64 pawnSpan[2][64];
U64 besideFileMasks[64];
U64 fileMasks[64];
U64 rankMasks[64];
// [pawn side][opposing king to move][pawn square]
U64 rule_of_square[2][2][64];
// [pawn side][pawn square]
U64 critical_squares[2][64];

void init_evaluation_masks() {
    int i,j;
    U64 bb, pawn;
    for (int i=0; i<64; i++) {
        pawn = (U64)1 << i;
        bb = northFill(pawn) & ~pawn;
        bb |= westOne(bb) | eastOne(bb);
        pawnSpan[0][i] = bb;
        critical_squares[0][i] = northOne(bb);
        bb = southFill(pawn) & ~pawn;
        bb |= westOne(bb) | eastOne(bb);
        pawnSpan[1][i] = bb;
        critical_squares[1][i] = southOne(bb);
        fileMasks[i] = fileFill(pawn);
        rankMasks[i] = (U64)0xFF << (i & 56);
        besideFileMasks[i] = westOne(fileMasks[i]) | eastOne(fileMasks[i]);
        // rule of square
        for (int pawn_side = WHITE; pawn_side <= BLACK; pawn_side++) {
            for (int king_to_move = _FALSE; king_to_move <= _TRUE; king_to_move++) {
                rule_of_square[pawn_side][king_to_move][i] = C64(0);
                for (int j=0; j<64; j++) {
                    int pawn_steps_to_queen = (pawn_side ? (i >> 3) : (7 - (i >> 3)));
                    int king_steps_to_queen_rank = (pawn_side ? (j >> 3) : (7 - (j >> 3)));
                    if ((king_steps_to_queen_rank - pawn_steps_to_queen - (king_to_move ? 1 : 0)) <= 0 &&
                    abs((j & 7) - (i & 7)) <= (pawn_steps_to_queen + (king_to_move ? 1 : 0)))
                        rule_of_square[pawn_side][king_to_move][i] |= (U64)1 << j;
                }
            }
        }
    }
}

static inline int count_isolated_pawns(U64 pawn_mask) {
    U64 open = ~fileFill(pawn_mask);
    pawn_mask &= eastOne(open);
    pawn_mask &= westOne(open);
    return popcount(pawn_mask);
}

static inline int count_doubled_pawns(U64 pawn_mask) {
    U64 mask = northOne(northFill(pawn_mask)) & southFill(pawn_mask);
    return popcount(mask);
}

static inline U64 white_passed_pawns(U64 wpawns, U64 bpawns) {
    U64 black_pawn_spans = southOne(southFill(bpawns));
    black_pawn_spans |= eastOne(black_pawn_spans) | westOne(black_pawn_spans);
    return wpawns & ~black_pawn_spans;
}

static inline U64 black_passed_pawns(U64 wpawns, U64 bpawns) {
    U64 white_pawn_spans = northOne(northFill(wpawns));
    white_pawn_spans |= eastOne(white_pawn_spans) | westOne(white_pawn_spans);
    return bpawns & ~white_pawn_spans;
}

static inline int count_white_backward_pawns(U64 wpawns) {
    U64 backfills = southOne(southFill(wpawns));
    U64 mask = westOne(backfills) | eastOne(backfills);
    wpawns &= mask;
    return popcount(wpawns); 
}

static inline int count_black_backward_pawns(U64 bpawns) {
    U64 backfills = northOne(northFill(bpawns));
    U64 mask = westOne(backfills) | eastOne(backfills);
    bpawns &= mask;
    return popcount(bpawns); 
}

// determine whether the position is a material draw
int material_draw() {
    if (!bitboards[P] && !bitboards[p]) {
        if (!bitboards[R] && !bitboards[r] && !bitboards[Q] && !bitboards[q]) {
            if (!bitboards[B] && !bitboards[b]) {
                if (popcount(bitboards[N]) < 3 && popcount(bitboards[n]) < 3)
                    return 1;
            } else if (!bitboards[N] && !bitboards[n]) {
                if (abs(popcount(bitboards[B]) - popcount(bitboards[b])) < 2)
                    return 1;
            } else if ((popcount(bitboards[N]) < 3 && !bitboards[B]) || (!((bitboards[B]) & (bitboards[B]-1)) && !bitboards[N])) {
                if ((popcount(bitboards[n]) < 3 && !bitboards[b]) || (!((bitboards[b]) & (bitboards[b]-1)) && !bitboards[n]))
                    return 1;
            }
        }
    }
    return 0;
}

// Determine whether the side on the move has the opposition
int has_opposition(int is_opponent_move, int king, int enemy_king) {
    int file_dist, rank_dist;
    file_dist = abs((king & 7) - (enemy_king & 7));
    rank_dist = abs((king >> 3) - (enemy_king >> 3));
    if (rank_dist < 2)
        return 1;   // No matter if it is us to move, if we are in this irregular situation, we may either gain opposition or get to a critical square
    if (!is_opponent_move) {
        // If we are on the move, we may make it an even distance away horizontally and vertically
        if (rank_dist & 1) {
            rank_dist--;
            is_opponent_move = 1;
        }
        if (file_dist & 1) {
            file_dist--;
            is_opponent_move = 1;
        }
    }
    if (!(file_dist & 1) && !(rank_dist & 1) && is_opponent_move)
        return 1;
    return 0;
}

int king_pawn_king(int side) {
    // Determines whether the side to move has a passer that can promote with the help of the king
    if (!((((bitboards[B] | bitboards[N])) | (bitboards[b] | bitboards[N]))) && !((((bitboards[Q] | bitboards[R])) | (bitboards[q] | bitboards[r])))) {
        // King and pawn endgame
        int wking_sq = bitscanForward(bitboards[K]);
        int bking_sq = bitscanForward(bitboards[k]);
        int kings_sq[2] = {wking_sq, bking_sq};
        if (bitboards[P + 6*side] && !bitboards[p - 6*side]) {  // If we have pawns, opponent doesn't
            bitboard = bitboards[P + 6*side];
            while (bitboard) {
                sq = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                // Rule of the square
                if (!(bitboards[k - 6 * side] & rule_of_square[side][side_to_move ^ side][sq]))
                    return 1;   // Decrease in score if opposing side is promoting
                // King should be infront of pawn
                if (((kings_sq[side] >> 3) - (sq >> 3)) * (1 - 2*side) <= 0)
                    continue;
                // For a rook pawn, king must be on adjacent file and closer to queening square
                int aFilePromotions[2] = {a8, a1};
                int hFilePromotions[2] = {h8, h1};
                if (!(sq & 7)) {    // A file
                    if ((bitboards[K + 6*side] & C64(0x0202020202020202)) &&
                    DISTANCE(kings_sq[side],aFilePromotions[side]) < DISTANCE(kings_sq[1 ^ side],aFilePromotions[side]))
                        return 1;
                    continue;
                }
                if (!(sq ^ 7)) {    // H file
                    if ((bitboards[K + 6*side] & C64(0x4040404040404040)) &&
                    DISTANCE(kings_sq[side],hFilePromotions[side]) < DISTANCE(kings_sq[1 ^ side],hFilePromotions[side]))
                        return 1;
                    continue;
                }
                U64 sixthRanks[2] = {C64(0xFF0000000000), C64(0xFF0000)};
                // Closer than (or equal) to pawn than enemy king, and either on 6th rank or on "critical squares"
                if (DISTANCE(kings_sq[side], sq) < DISTANCE(kings_sq[1^side], sq)) {
                    if (bitboards[K + 6 * side] & (sixthRanks[side] | critical_squares[side][sq])) {
                        return 1;
                    }
                }
                // King is one square infront of the pawn and has the opposition
                if (bitboards[K + 6 * side] & (pawnSpan[side][sq] ^ critical_squares[side][sq])) {
                    if (has_opposition(side ^ side_to_move,kings_sq[side],kings_sq[1 ^ side]))
                        return 1;
                }
            }
        }
    }
    return 0;
}

int eval_king(int side, int opponent_material, U64 open_files, U64 opponent_semiopen_files) {
    int score = 0;
    U64 king_bb = bitboards[K + 6*side];
    int king_square = bitscanForward(king_bb);
    U64 pawn_bb = bitboards[P + 6*side];
    U64 our_side = side ? ~white_half : white_half;
    pawn_bb &= our_side;
    if (king_bb & eastOne(kingside)) {
        // Only g and h files, i.e. castled kingside
        pawn_bb &= kingside;
        score += 15 * popcount(pawn_bb & secondSeventh);
        score += 6 * popcount(pawn_bb & thirdSixth);
        score += (-90 + 30 * popcount(pawn_bb));
    } else if (king_bb & queenside) {
        pawn_bb &= queenside;
        score += 15 * popcount(pawn_bb & secondSeventh);
        score += 6 * popcount(pawn_bb & thirdSixth);
        score += (-90 + 30 * popcount(pawn_bb));
    } else {
        // Penalty for open files near the king
        score -= 30 * popcount(open_files & besideFileMasks[king_square] & rankMasks[king_square]);
    }
    if (fileMasks[king_square] & open_files)
        score -= 30;
    else if (fileMasks[king_square] & opponent_semiopen_files)
        score -= 15;
    score *= opponent_material;
    score /= 3100;
    return score;
}

// Linear evaluation function - Where all of the evaluation parameters are found, besides the values used for tapered evaluation (Arguably the most important)
void linear_eval(int *mg_score, int *eg_score) {
    int pc, sq;
    U64 bitboard;
    int eval_mg[2];
    int eval_eg[2];
    eval_mg[WHITE] = 0;
    eval_mg[BLACK] = 0;
    eval_eg[WHITE] = 0;
    eval_eg[BLACK] = 0;
    for (pc=K;pc<=Q;pc++) {
        if (pc==P)
            continue;
        bitboard = bitboards[pc];
        while (bitboard) {
            sq = bitscanForward(bitboard);
            clear_ls1b(bitboard);
            eval_mg[WHITE] += piece_phase_values[MG][pc];
            eval_mg[WHITE] += pstables[MG][pc][FLIP(sq)];
            eval_eg[WHITE] += piece_phase_values[EG][pc];
            eval_eg[WHITE] += pstables[EG][pc][FLIP(sq)];
        }
        bitboard = bitboards[pc + 6];
        while (bitboard) {
            sq = bitscanForward(bitboard);
            clear_ls1b(bitboard);
            eval_mg[BLACK] += piece_phase_values[MG][pc];
            eval_mg[BLACK] += pstables[MG][pc][sq];
            eval_eg[BLACK] += piece_phase_values[EG][pc];
            eval_eg[BLACK] += pstables[EG][pc][sq];
        }
    }
    // Pawn structure considerations
    bitboard = bitboards[P];
    while (bitboard) {
        sq = bitscanForward(bitboard);
        clear_ls1b(bitboard);
        eval_mg[WHITE] += piece_phase_values[MG][P];
        eval_mg[WHITE] += pstables[MG][P][FLIP(sq)];
        eval_eg[WHITE] += piece_phase_values[EG][P];
        eval_eg[WHITE] += pstables[EG][P][FLIP(sq)];
        if (!(pawnSpan[WHITE][sq] & bitboards[p])) {    // Passer
            eval_mg[WHITE] += passed_pawn_bonus[MG][WHITE][sq >> 3];
            eval_eg[WHITE] += passed_pawn_bonus[EG][WHITE][sq >> 3];
        }
        if (!(besideFileMasks[sq] & bitboards[P])) {    // Isolated
            eval_mg[WHITE] -= isolated_pawn_penalty[MG];
            eval_eg[WHITE] -= isolated_pawn_penalty[EG];
        }
        if ((besideFileMasks[sq] & bitboards[P]) && !(northOne(pawnSpan[BLACK][sq]) & bitboards[p] & ~fileMasks[sq])) {  // backward
            eval_mg[WHITE] -= backward_pawn_penalty[MG];
            eval_eg[WHITE] -= backward_pawn_penalty[EG];
        }
    }
    int count = count_doubled_pawns(bitboards[P]);
    eval_mg[WHITE] -= doubled_pawn_penalty[MG] * count;
    eval_eg[WHITE] -= doubled_pawn_penalty[EG] * count;

    bitboard = bitboards[p];
    while (bitboard) {
        sq = bitscanForward(bitboard);
        clear_ls1b(bitboard);
        eval_mg[BLACK] += piece_phase_values[MG][P];
        eval_mg[BLACK] += pstables[MG][P][sq];
        eval_eg[BLACK] += piece_phase_values[EG][P];
        eval_eg[BLACK] += pstables[EG][P][sq];
        if (!(pawnSpan[BLACK][sq] & bitboards[P])) {    // Passer
            eval_mg[BLACK] += passed_pawn_bonus[MG][BLACK][sq >> 3];
            eval_eg[BLACK] += passed_pawn_bonus[EG][BLACK][sq >> 3];
        }
        if (!(besideFileMasks[sq] & bitboards[p])) {    // Isolated
            eval_mg[BLACK] -= isolated_pawn_penalty[MG];
            eval_eg[BLACK] -= isolated_pawn_penalty[EG];
        }
        if ((besideFileMasks[sq] & bitboards[p]) && !(southOne(pawnSpan[WHITE][sq]) & bitboards[p] & ~fileMasks[sq])) {  // backward
            eval_mg[BLACK] -= backward_pawn_penalty[MG];
            eval_eg[BLACK] -= backward_pawn_penalty[EG];
        }
    }
    count = count_doubled_pawns(bitboards[p]);
    eval_mg[BLACK] -= doubled_pawn_penalty[MG] * count;
    eval_eg[BLACK] -= doubled_pawn_penalty[EG] * count;

    if (!bitboards[P])
        eval_eg[WHITE] -= NO_PAWNS_PENALTY;
    if (!bitboards[p])
        eval_eg[BLACK] -= NO_PAWNS_PENALTY;

    U64 open_files = ~fileFill(bitboards[p] | bitboards[P]);
    U64 white_semiopen_files = ~fileFill(bitboards[P]) & ~open_files;
    U64 black_semiopen_files = ~fileFill(bitboards[p]) & ~open_files;
    // King safety
    eval_mg[WHITE] += eval_king(WHITE, material_count[BLACK], open_files, black_semiopen_files);
    eval_mg[BLACK] += eval_king(BLACK, material_count[WHITE],open_files, white_semiopen_files);

    // Rooks on open and semi-open files
    count = popcount(bitboards[R] & open_files);
    eval_mg[WHITE] += count * rook_open_file_bonus[MG];
    eval_eg[WHITE] += count * rook_open_file_bonus[EG];
    count = popcount(bitboards[R] & white_semiopen_files);
    eval_mg[WHITE] += count * rook_semi_open_bonus[MG];
    eval_mg[WHITE] += count * rook_semi_open_bonus[MG];

    count = popcount(bitboards[r] & open_files);
    eval_mg[BLACK] += count * rook_open_file_bonus[MG];
    eval_eg[BLACK] += count * rook_open_file_bonus[EG];
    count = popcount(bitboards[r] & black_semiopen_files);
    eval_mg[BLACK] += count * rook_semi_open_bonus[MG];
    eval_mg[BLACK] += count * rook_semi_open_bonus[MG];

    *mg_score = side_to_move ? (eval_mg[BLACK] - eval_mg[WHITE]) : (eval_mg[WHITE] - eval_mg[BLACK]);
    *eg_score = side_to_move ? (eval_eg[BLACK] - eval_eg[WHITE]) : (eval_eg[WHITE] - eval_eg[BLACK]);

    if (king_pawn_king(WHITE))
        *eg_score += UNSTOPPABLE_PASSER;
    else if (king_pawn_king(BLACK))
        *eg_score -= UNSTOPPABLE_PASSER;
}

// Evaluate the current position. If the posititon is good for the side to move, returns a + score
int eval(int alpha, int beta) {

    // Lazy evaluation based on piece material. If the material score (simple) would put us outside the alpha beta bounds by >600, we return that bound
    /* int lazy_score = side_to_move ? (material_count[BLACK] - material_count[WHITE]) : (material_count[WHITE] - material_count[BLACK]);
    if (lazy_score + 600 < alpha)
        return alpha;
    if (lazy_score - 600 > beta)
        return beta; */


    int white_piece_scores = 0;
    int black_piece_scores = 1;
    // Get the game phase score
    for (int piece=P; piece <= Q; piece++) {
        white_piece_scores += popcount(bitboards[piece]) * piece_phase_values[MG][piece];
        black_piece_scores += popcount(bitboards[6 + piece]) * piece_phase_values[MG][piece];
    }
    int game_phase = white_piece_scores + black_piece_scores;
    const int opening_phase_score = 6192;
    const int endgame_phase_score = 518;

    int mg_score;
    int eg_score;

    linear_eval(&mg_score, &eg_score);

    // Material draws
    if (!bitboards[P] && !bitboards[p]) {
        if (material_draw())
            return 0;
        else if (!bitboards[Q] && !bitboards[q]) {
            if (!((bitboards[R]) & (bitboards[R]-1)) && !((bitboards[r]) & (bitboards[r]-1)) && bitboards[R] && bitboards[r]) {
                if ((popcount(bitboards[B] | bitboards[N]) < 1 && !(bitboards[n] | bitboards[b])) || (popcount(bitboards[b] | bitboards[n]) < 1 && !(bitboards[N] | bitboards[B])))
                    return eg_score / 2;
            } else if (!((bitboards[R]) & (bitboards[R]-1)) && bitboards[R] && !bitboards[r]) {
                if (!(bitboards[B] | bitboards[N]) && ((bitboards[n] | bitboards[b]) && popcount(bitboards[n] | bitboards[b]) <= 2))
                    return eg_score / 2;
            } else if (!((bitboards[r]) & (bitboards[r]-1)) && bitboards[r] && !bitboards[R]) {
                if (!(bitboards[b] | bitboards[n]) && ((bitboards[B] | bitboards[N]) && popcount(bitboards[N] | bitboards[B]) <= 2))
                    return eg_score / 2;
            }
        }
    }

    if (game_phase > opening_phase_score)
        return mg_score;
    else if (game_phase < endgame_phase_score)
        return eg_score;
    else
        return (mg_score * game_phase + eg_score * (opening_phase_score - game_phase)) / opening_phase_score;
}


// Static exchange evaluation

// Mask of pieces that directly attack a given square
static inline U64 attacks_to(int target) {
    U64 occ = occupancies[BOTH];
    U64 attacks = (pawn_attack_table[WHITE][target] & bitboards[p]) | (pawn_attack_table[BLACK][target] & bitboards[P]);
    attacks |= knight_attack_table[target] & (bitboards[N] | bitboards[n]);
    attacks |= king_attack_table[target] & (bitboards[k] | bitboards[K]);
    U64 bishops = (bitboards[b] | bitboards[B]) | (bitboards[q] | bitboards[Q]);
    U64 rooks = (bitboards[r] | bitboards[R]) | (bitboards[q] | bitboards[Q]);
    if (bishopAttackTable[target][0] & bishops)
        attacks |= (bishopAttacks(target, occ) & bishops);
    if (rookAttackTable[target][0] & rooks)
        attacks |= (rookAttacks(target, occ) & rooks);
    return attacks;
}

int SEE(int side, U16 move) {
    U64 attacks, temp = 0, toccupied = occupancies[BOTH];
    U64 bsliders = (bitboards[b] | bitboards[B]) | (bitboards[q] | bitboards[Q]);
    U64 rsliders = (bitboards[r] | bitboards[R]) | (bitboards[q] | bitboards[Q]);
    int attacked_piece, piece, nc=1, see_list[32];
    int source = move_source(move), target = move_target(move);

    attacks = attacks_to(target);
    attacked_piece = positive_simple_piece_values[piece_on_square[target]];
    side ^= 1;
    see_list[0] = attacked_piece;
    toccupied &= ~((U64)1 << source);
    piece = piece_on_square[source];
    attacked_piece = positive_simple_piece_values[piece];
    int piece_type = piece % 6;
    if (piece & 1)     // Pawn, bishop, queen
        attacks |= bishopAttacks(target, toccupied) & bsliders;
    if (piece_type != K && (piece_type == P || piece_type > B))
        attacks |= rookAttacks(target, toccupied) & rsliders;
    // Pick out least valuable attacker
    for (attacks &= toccupied; attacks; attacks &= toccupied) {
        for (piece = P; piece <= k; piece++) {
            if ((temp = bitboards[(piece % 6) + 6 * side] & attacks)) {
                break;  // Least valuable
            }
        }
        if (piece > k)
            break;
        piece = piece % 6;
        toccupied ^= (temp & -temp);    // Clear of the attacker's square
        if (piece & 1)
            attacks |= bishopAttacks(target, toccupied) & bsliders;
        if (piece_type != K && piece_type > B)
            attacks |= rookAttacks(target, toccupied) & rsliders;
        see_list[nc] = -see_list[nc - 1] + attacked_piece;
        attacked_piece = positive_simple_piece_values[piece];
        if (see_list[nc++] - attacked_piece > 0)
            break;
        side ^= 1;    
    }
    while (--nc)
        see_list[nc - 1] = (-see_list[nc - 1] > see_list[nc]) ? see_list[nc - 1] : -see_list[nc];
    return see_list[0];
}