#include "eval.h"
#include "tt.h"

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

static inline U64 wpawn_attack_mask(U64 bb) {
    U64 aside = westOne(bb) | eastOne(bb);
    return northOne(aside);
}

static inline U64 bpawn_attack_mask(U64 bb) {
    U64 aside = westOne(bb) | eastOne(bb);
    return southOne(aside);
}

// Piece evaluations

static inline void eval_knight(board_t* board, piece_eval_t* peval, U64 oppPawn, enumSquare kn, enumSquare oppKing, enumSide side) {
    peval->trop = DISTANCE(kn,oppKing);
    U64 bb = (knightAttackTable[kn] & ~board->occupancies[side]) & ~(oppPawn & (~board->occupancies[BOTH] | board->bitboards[p - 6*side]));
    peval->mob = popcount(bb);
}

static inline void eval_bishop(board_t* board, piece_eval_t* peval, U64 oppPawn, enumSquare bsh, enumSquare oppKing, enumSide side) {
    peval->trop = DISTANCE(bsh,oppKing);
    U64 bb = (bishopAttacks(bsh,board->occupancies[BOTH]) & ~board->occupancies[side]) & ~(oppPawn & (~board->occupancies[BOTH] | board->bitboards[p - 6*side]));
    peval->mob = popcount(bb);
}

static inline void eval_rook(board_t* board, piece_eval_t* peval, U64 oppPawn, enumSquare rk, enumSquare oppKing, enumSide side) {
    peval->trop = DISTANCE(rk,oppKing);
    U64 bb = (rookAttacks(rk,board->occupancies[BOTH]) & ~board->occupancies[side]) & ~(oppPawn & (~board->occupancies[BOTH] | board->bitboards[p - 6*side]));
    peval->mob = popcount(bb);
}

static inline void eval_queen(board_t* board, piece_eval_t* peval, U64 oppPawn, enumSquare qn, enumSquare oppKing, enumSide side) {
    peval->trop = DISTANCE(qn,oppKing);
    U64 bb = (rookAttacks(qn,board->occupancies[BOTH]) | bishopAttacks(qn,board->occupancies[BOTH])) & ~board->occupancies[side];
    peval->mob = popcount(bb);
}

// material_scores[phase][piece]
const int material_scores[2][6] = {
    {0,95,300,320,460,920,},
    {0,100,280,290,500,930,},
};

const int doubled_pawn_penalty[2] = {15,20};
const int isolated_pawn_penalty[2] = {15,12};
const int backward_pawn_penalty[2] = {15,12};
const int passed_pawn_bonus[8] = {0, 10, 20, 30, 40, 50, 60, 70};
const int semiopen_bonus[2] = {12,12};
const int open_bonus[2] = {20,20};

const int no_pawn_penalty = 150;
const int bad_bishop[2] = {25,25};
const int trapped_rook[2] = {40,40};

const int undeveloped_penalty = 12;
const U64 firstRank = C64(0xFF);
const U64 lastRank = C64(0xFF00000000000000);

const int knight_mobility[2] = {3,4};
const int knight_tropism[2] = {3,3};
const int bishop_mobility[2] = {3,4};
const int bishop_tropism[2] = {2,1};
const int rook_mobility[2] = {1,2};
const int rook_tropism[2] = {1,1};
const int queen_mobility[2] = {1,1};
const int queen_tropism[2] = {4,2};

// pstables[phase][piece][sq] from white perspective
const int pstables[2][6][64] = {
    {{-10, 10, 0, -20, 0, -10, 10, 0,
    -10,-10,-15,-25,-20,-20,-10,-5,
    -15,-15,-20,-35,-35,-20,-15,-15,
    -25,-25,-30,-40,-40,-30,-25,-25,
    -40,-40,-40,-40,-40,-40,-40,-40,
    -50,-50,-50,-50,-50,-50,-50,-50,
    -60,-60,-60,-60,-60,-60,-60,-60,
    -70,-70,-70,-70,-70,-70,-70,-70,},
    {0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -5, -20, -20, 0, 0, 0,
    3, 3, 3, -10, -10, -3, 3, 3,
    5, 0, 10, 10, 10, -5, 0, 0,
    10, 5, 15, 20, 20, 5, 5, 5,
    13, 13, 20, 25, 25, 20, 13, 13,
    20, 20, 30, 40, 40, 30, 20, 20,
    0, 0, 0, 0, 0, 0, 0, 0,},
    {-20, -15, -10, -10, -10, -10, -20, -25,
    -15, -10, -10, 0, 0, -10, -10, -15,
    -5, 0, 8, 5, 5, 8, 0, -5,
    0, 3, 12, 12, 12, 12, 3, 0,
    5, 8, 15, 25, 25, 15, 8, 5,
    0, 10, 25, 35, 35, 25, 10, 0,
    0, 0, 10, 10, 10, 10, 0, 0,
    -40, -30, -20, -15, -15, -20, -30, -40,},
    {-5, -10, -5, -10, -10, -5, -10, -5,
    3, 5, 3, 0, 0, 3, 5, 3,
    -2, 5, 5, 8, 8, 5, 5, -2,
    6, 10, 15, 15, 15, 15, 10, 6,
    6, 10, 15, 15, 15, 15, 10, 6,
    -2, 5, 5, 8, 8, 5, 5, -2,
    3, 5, 3, 0, 0, 3, 5, 3,
    -5, -10, -5, -10, -10, -5, -10, -5,},
    {-15, -10, 5, 5, 5, 0, -10, -15,
    -10, -5, 5, 10, 10, 0, -5, -10,
    -10, -5, 0, 5, 5, 0, -5, -10,
    -10, -5, 0, 5, 5, 0, -5, -10,
    -10, -5, 0, 5, 5, 0, -5, -10,
    0, 0, 10, 10, 10, 10, 0, 0,
    30, 35, 40, 50, 50, 40, 35, 30,
    30, 35, 40, 50, 50, 40, 35, 30,},
    {-15, -10, 0, 0, 0, 0, -10, -15,
    -20, -5, 5, 5, 5, 5, -5, -20,
    -10, 8, 5, 5, 5, 5, -5, -10,
    5, 0, 5, 8, 8, 5, 0, 5,
    5, 0, 5, 8, 8, 5, 0, 5,
    -10, 0, 5, 5, 5, 5, -5, -10,
    -20, -5, 5, 5, 5, 5, -5, -20,
    -15, -10, 0, 0, 0, 0, -10, -15,},
    },
    {{-30, -20, -10, 0, 0, -10, -20, -30,
    -20, -10, 0, 10, 10, 0, -10, -20,
    -10, 0, 10, 20, 20, 10, 0, -10,
    0, 10, 20, 30, 30, 20, 10, 0,
    0, 10, 20, 30, 30, 20, 10, 0,
    -10, 0, 10, 20, 20, 10, 0, -10,
    -20, -10, 0, 10, 10, 0, -10, -20,
    -30, -20, -10, 0, 0, -10, -20, -30,},
    {0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -10, -10, 0, 0, 0,
    0, 0, 0, 5, 5, 0, 0, 0,
    5, 5, 10, 20, 20, 10, 5, 5,
    10, 10, 10, 20, 20, 10, 10, 10,
    20, 20, 30, 30, 30, 30, 20, 20,
    30, 30, 40, 50, 50, 40, 30, 30,
    0, 0, 0, 0, 0, 0, 0, 0,},
    {-15, -10, -5, 0, 0, -5, -10, -15,
     -10, -5, 0, 5, 5, 0, -5, -10,
     -5, 0, 5, 10, 10, 5, 0, -5,
     0, 5, 10, 15, 15, 10, 5, 0,
     0, 5, 10, 15, 15, 10, 5, 0,
     -5, 0, 5, 10, 10, 5, 0, -5,
     -10, -5, 0, 5, 5, 0, -5, -10,
     -15, -10, -5, 0, 0, -5, -10, -15,},
    {-5, -10, -5, -10, -10, -5, -10, -5,
    3, 5, 3, 0, 0, 3, 5, 3,
    -2, 5, 5, 8, 8, 5, 5, -2,
    6, 10, 15, 15, 15, 15, 10, 6,
    6, 10, 15, 15, 15, 15, 10, 6,
    -2, 5, 5, 8, 8, 5, 5, -2,
    3, 5, 3, 0, 0, 3, 5, 3,
    -5, -10, -5, -10, -10, -5, -10, -5,},
    {0, 0, 0, 5, 5, 0, 0, 0,
    0, 0, 0, 5, 5, 0, 0, 0,
    0, 0, 0, 5, 5, 0, 0, 0,
    0, 0, 0, 5, 5, 0, 0, 0,
    0, 0, 0, 5, 5, 0, 0, 0,
    5, 5, 5, 10, 10, 5, 5, 5,
    25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25,},
    {-15, -10, 0, 0, 0, 0, -10, -15,
    -20, -5, 5, 5, 5, 5, -5, -20,
    -10, 8, 5, 5, 5, 5, -5, -10,
    5, 0, 5, 8, 8, 5, 0, 5,
    5, 0, 5, 8, 8, 5, 0, 5,
    -10, 0, 5, 5, 5, 5, -5, -10,
    -20, -5, 5, 5, 5, 5, -5, -20,
    -15, -10, 0, 0, 0, 0, -10, -15,},},
};

int evaluate(board_t* board) {
    int mgscore[2], egscore[2], pcscore[2], score, i, pc, sq, col, mat, eg, mg, ct;
    U64 bb, mask, wpmask, bpmask;
    piece_eval_t peval;
    enumSquare WK, BK;

    if ((score = probe_eval_tt(board->hash)) != INVALID) {
        return score;
    }

    WK = bitscanForward(board->bitboards[K]);
    BK = bitscanForward(board->bitboards[k]);

    pcscore[0] = pcscore[1] = 0;
    egscore[0] = egscore[1] = 0;
    mgscore[0] = mgscore[1] = 0;

    wpmask = wpawn_attack_mask(board->bitboards[P]);
    bpmask = bpawn_attack_mask(board->bitboards[p]);
    // Knights
    bb = board->bitboards[N];
    while (bb) {
        sq = bitscanForward(bb);
        POP_LS1B(bb);
        pcscore[WHITE] += material_scores[EG][N];
        mgscore[WHITE] += material_scores[MG][N];
        mgscore[WHITE] += pstables[MG][N][sq];
        egscore[WHITE] += pstables[EG][N][sq];

        eval_knight(board, &peval, bpmask, sq, BK, WHITE);
        mgscore[BLACK] -= knight_tropism[MG] * (peval.trop - 7);
        egscore[BLACK] -= knight_tropism[EG] * (peval.trop - 7);
        mgscore[WHITE] += knight_mobility[MG] * (peval.mob - 4);
        egscore[WHITE] += knight_mobility[EG] * (peval.mob - 4);
    }

    bb = board->bitboards[n];
    while (bb) {
        sq = bitscanForward(bb);
        POP_LS1B(bb);
        pcscore[BLACK] += material_scores[EG][N];
        mgscore[BLACK] += material_scores[MG][N];
        mgscore[BLACK] += pstables[MG][N][56^sq];
        egscore[BLACK] += pstables[EG][N][56^sq];

        eval_knight(board, &peval, wpmask, sq, WK, BLACK);
        mgscore[WHITE] -= knight_tropism[MG] * (peval.trop - 7);
        egscore[WHITE] -= knight_tropism[EG] * (peval.trop - 7);
        mgscore[BLACK] += knight_mobility[MG] * (peval.mob - 4);
        egscore[BLACK] += knight_mobility[EG] * (peval.mob - 4);
    }

    // Bishops
    bb = board->bitboards[B];
    while (bb) {
        sq = bitscanForward(bb);
        POP_LS1B(bb);
        pcscore[WHITE] += material_scores[EG][B];
        mgscore[WHITE] += material_scores[MG][B];
        mgscore[WHITE] += pstables[MG][B][sq];
        egscore[WHITE] += pstables[EG][B][sq];

        eval_bishop(board, &peval, bpmask, sq, BK, WHITE);
        mgscore[BLACK] -= bishop_tropism[MG] * (peval.trop - 7);
        egscore[BLACK] -= bishop_tropism[EG] * (peval.trop - 7);
        mgscore[WHITE] += bishop_mobility[MG] * (peval.mob - 7);
        egscore[WHITE] += bishop_mobility[EG] * (peval.mob - 7);

        if (peval.mob < 3) {mgscore[WHITE] -= bad_bishop[MG];}
        if (peval.mob < 6) {egscore[WHITE] -= bad_bishop[EG];} 
    }

    bb = board->bitboards[b];
    while (bb) {
        sq = bitscanForward(bb);
        POP_LS1B(bb);
        pcscore[BLACK] += material_scores[EG][B];
        mgscore[BLACK] += material_scores[MG][B];
        mgscore[BLACK] += pstables[MG][B][56^sq];
        egscore[BLACK] += pstables[EG][B][56^sq];

        eval_bishop(board, &peval, wpmask, sq, WK, BLACK);
        mgscore[WHITE] -= bishop_tropism[MG] * (peval.trop - 7);
        egscore[WHITE] -= bishop_tropism[EG] * (peval.trop - 7);
        mgscore[BLACK] += bishop_mobility[MG] * (peval.mob - 7);
        egscore[BLACK] += bishop_mobility[EG] * (peval.mob - 7);

        if (peval.mob < 3) {mgscore[BLACK] -= bad_bishop[MG];}
        if (peval.mob < 6) {egscore[BLACK] -= bad_bishop[EG];} 
    }

    // Development

    mgscore[WHITE] -= popcount((board->bitboards[N] | board->bitboards[B]) & firstRank) * undeveloped_penalty;
    mgscore[BLACK] -= popcount((board->bitboards[b] | board->bitboards[b]) & lastRank) * undeveloped_penalty;

    // Rooks

    mask = ~fileFill(board->bitboards[P] | board->bitboards[p]);

    bb = board->bitboards[R];
    while (bb) {
        sq = bitscanForward(bb);
        POP_LS1B(bb);
        pcscore[WHITE] += material_scores[EG][R];
        mgscore[WHITE] += material_scores[MG][R];
        mgscore[WHITE] += pstables[MG][R][sq];
        egscore[WHITE] += pstables[EG][R][sq];

        eval_rook(board, &peval, bpmask, sq, BK, WHITE);
        mgscore[BLACK] -= rook_tropism[MG] * (peval.trop - 7);
        egscore[BLACK] -= rook_tropism[EG] * (peval.trop - 7);
        mgscore[WHITE] += rook_mobility[MG] * (peval.mob - 7);
        egscore[WHITE] += rook_mobility[EG] * (peval.mob - 7);

        if (peval.mob < 3) {mgscore[WHITE] -= trapped_rook[MG];}
        if (peval.mob < 6) {egscore[WHITE] -= trapped_rook[EG];} 
    }

    bb = mask & board->bitboards[R];
    ct = popcount(bb);
    mgscore[WHITE] += ct * open_bonus[MG];
    egscore[WHITE] += ct * open_bonus[EG];

    bb = ~fileFill(board->bitboards[P]) & board->bitboards[R] & ~mask;
    ct = popcount(bb);
    mgscore[WHITE] += ct * semiopen_bonus[MG];
    egscore[WHITE] += ct * semiopen_bonus[EG];


    bb = board->bitboards[r];
    while (bb) {
        sq = bitscanForward(bb);
        POP_LS1B(bb);
        pcscore[BLACK] += material_scores[EG][R];
        mgscore[BLACK] += material_scores[MG][R];
        mgscore[BLACK] += pstables[MG][R][56^sq];
        egscore[BLACK] += pstables[EG][R][56^sq];

        eval_rook(board, &peval, wpmask, sq, WK, BLACK);
        mgscore[WHITE] -= rook_tropism[MG] * (peval.trop - 7);
        egscore[WHITE] -= rook_tropism[EG] * (peval.trop - 7);
        mgscore[BLACK] += rook_mobility[MG] * (peval.mob - 7);
        egscore[BLACK] += rook_mobility[EG] * (peval.mob - 7);

        if (peval.mob < 3) {mgscore[BLACK] -= trapped_rook[MG];}
        if (peval.mob < 6) {egscore[BLACK] -= trapped_rook[EG];} 
    }

    bb = mask & board->bitboards[r];
    ct = popcount(bb);
    mgscore[BLACK] += ct * open_bonus[MG];
    egscore[BLACK] += ct * open_bonus[EG];

    bb = ~fileFill(board->bitboards[p]) & board->bitboards[r] & ~mask;
    ct = popcount(bb);
    mgscore[BLACK] += ct * semiopen_bonus[MG];
    egscore[BLACK] += ct * semiopen_bonus[EG];

    // Queens

    bb = board->bitboards[Q];
    while (bb) {
        sq = bitscanForward(bb);
        POP_LS1B(bb);
        pcscore[WHITE] += material_scores[EG][Q];
        mgscore[WHITE] += material_scores[MG][Q];
        mgscore[WHITE] += pstables[MG][Q][sq];
        egscore[WHITE] += pstables[EG][Q][sq];

        eval_queen(board, &peval, bpmask, sq, BK, WHITE);
        mgscore[BLACK] -= queen_tropism[MG] * (peval.trop - 7);
        egscore[BLACK] -= queen_tropism[EG] * (peval.trop - 7);
        mgscore[WHITE] += queen_mobility[MG] * (peval.mob - 7);
        egscore[WHITE] += queen_mobility[EG] * (peval.mob - 7);
    }

    bb = board->bitboards[q];
    while (bb) {
        sq = bitscanForward(bb);
        POP_LS1B(bb);
        pcscore[BLACK] += material_scores[EG][Q];
        mgscore[BLACK] += material_scores[MG][Q];
        mgscore[BLACK] += pstables[MG][Q][56^sq];
        egscore[BLACK] += pstables[EG][Q][56^sq];

        eval_queen(board, &peval, wpmask, sq, WK, BLACK);
        mgscore[WHITE] -= queen_tropism[MG] * (peval.trop - 7);
        egscore[WHITE] -= queen_tropism[EG] * (peval.trop - 7);
        mgscore[BLACK] += queen_mobility[MG] * (peval.mob - 7);
        egscore[BLACK] += queen_mobility[EG] * (peval.mob - 7);
    }

    // Pawns
    bb = board->bitboards[P];
    while (bb) {
        sq = bitscanForward(bb);
        POP_LS1B(bb);
        pcscore[WHITE] += material_scores[EG][P];
        mgscore[WHITE] += material_scores[MG][P];
        mgscore[WHITE] += pstables[MG][P][sq];
        egscore[WHITE] += pstables[EG][P][sq];
    }

    bb = board->bitboards[p];
    while (bb) {
        sq = bitscanForward(bb);
        POP_LS1B(bb);
        pcscore[BLACK] += material_scores[EG][P];
        mgscore[BLACK] += material_scores[MG][P];
        mgscore[BLACK] += pstables[MG][P][56^sq];
        egscore[BLACK] += pstables[EG][P][56^sq];
    }

    ct = count_doubled_pawns(board->bitboards[P]);
    mgscore[WHITE] -= ct*doubled_pawn_penalty[MG];
    egscore[WHITE] -= ct*doubled_pawn_penalty[EG];
    ct = count_doubled_pawns(board->bitboards[p]);
    mgscore[BLACK] -= ct*doubled_pawn_penalty[MG];
    egscore[BLACK] -= ct*doubled_pawn_penalty[EG];

    ct = count_isolated_pawns(board->bitboards[P]);
    mgscore[WHITE] -= ct*isolated_pawn_penalty[MG];
    egscore[WHITE] -= ct*isolated_pawn_penalty[EG];
    ct = count_isolated_pawns(board->bitboards[p]);
    mgscore[BLACK] -= ct*isolated_pawn_penalty[MG];
    egscore[BLACK] -= ct*isolated_pawn_penalty[EG];

    bb = white_passed_pawns(board->bitboards[P], board->bitboards[p]);
    while (bb) {
        sq = bitscanForward(bb);
        POP_LS1B(bb);
        egscore[WHITE] += passed_pawn_bonus[sq / 8];
    }
    bb = black_passed_pawns(board->bitboards[P], board->bitboards[p]);
    while (bb) {
        sq = bitscanForward(bb);
        POP_LS1B(bb);
        egscore[BLACK] += passed_pawn_bonus[7 - (sq / 8)];
    }


    if (!board->bitboards[P]) {egscore[WHITE] -= no_pawn_penalty;}
    if (!board->bitboards[p]) {egscore[BLACK] -= no_pawn_penalty;}

    egscore[0] += pcscore[0];
    egscore[1] += pcscore[1];

    mg = mgscore[0] - mgscore[1];
    eg = egscore[0] - egscore[1];
    mat = pcscore[0] + pcscore[1];

    if (mat > MGTHRESH) {score = mg;}
    else if (mat < EGTHRESH) {score = eg;}
    else {
        score = ((mat - EGTHRESH)*mg + (MGTHRESH - mat)*eg) / (MGTHRESH - EGTHRESH);
    }

    store_eval_tt(board->hash,score*(1-2*board->side));

    return score*(1-2*board->side);
}

bool is_draw(board_t* board, search_info_t* si) {
    // Draw by repetition or 50 move rule
    if (board->rule50 >= 100) {return true;}
    board->game_history[board->hply + si->ply] = board->hash;
    
    for (int x = board->hply + si->ply - 2;x>=MAX(0,board->hply + si->ply - board->rule50);x-=2) {
        if (board->hash == board->game_history[x]) {return true;}
    }
    return false;
}