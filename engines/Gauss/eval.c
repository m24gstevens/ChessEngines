#include "eval.h"

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

const int undeveloped_penalty = 12;
const U64 firstRank = C64(0xFF);
const U64 lastRank = C64(0xFF00000000000000);

// pstables[phase][piece][sq] from white perspective
const int pstables[2][6][64] = {
    {{-10, 10, 15, -15, 0, 5, 10, 0,
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
    U64 bb, mask;

    pcscore[0] = pcscore[1] = 0;
    egscore[0] = egscore[1] = 0;
    mgscore[0] = mgscore[1] = 0;

    for (int i=0;i<64;i++) {
        pc = board->squares[i];
        if (pc == _) {continue;}
        col = pc / 6;
        sq = (col ? 56^i : i);

        pcscore[col] += material_scores[EG][pc % 6];
        mgscore[col] += material_scores[MG][pc % 6];
        mgscore[col] += pstables[MG][pc % 6][sq];
        egscore[col] += pstables[EG][pc % 6][sq];
    }
    egscore[0] += pcscore[0];
    egscore[1] += pcscore[1];

    // Doubled, isolated, backward
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

    mask = ~fileFill(board->bitboards[P] | board->bitboards[p]);

    bb = mask & board->bitboards[R];
    ct = popcount(bb);
    mgscore[WHITE] += ct * open_bonus[MG];
    egscore[WHITE] += ct * open_bonus[EG];

    bb = mask & board->bitboards[r];
    ct = popcount(bb);
    mgscore[BLACK] += ct * open_bonus[MG];
    egscore[BLACK] += ct * open_bonus[EG];

    bb = ~fileFill(board->bitboards[P]) & board->bitboards[R] & ~mask;
    ct = popcount(bb);
    mgscore[WHITE] += ct * semiopen_bonus[MG];
    egscore[WHITE] += ct * semiopen_bonus[EG];

    bb = ~fileFill(board->bitboards[p]) & board->bitboards[r] & ~mask;
    ct = popcount(bb);
    mgscore[BLACK] += ct * semiopen_bonus[MG];
    egscore[BLACK] += ct * semiopen_bonus[EG];

    mgscore[WHITE] -= popcount((board->bitboards[N] | board->bitboards[B]) & firstRank) * undeveloped_penalty;
    mgscore[BLACK] -= popcount((board->bitboards[b] | board->bitboards[b]) & lastRank) * undeveloped_penalty;

    if (!board->bitboards[P]) {egscore[WHITE] -= no_pawn_penalty;}
    if (!board->bitboards[p]) {egscore[BLACK] -= no_pawn_penalty;}


    mg = mgscore[0] - mgscore[1];
    eg = egscore[0] - egscore[1];
    mat = pcscore[0] + pcscore[1];

    if (mat > MGTHRESH) {score = mg;}
    else if (mat < EGTHRESH) {score = eg;}
    else {
        score = ((mat - EGTHRESH)*mg + (MGTHRESH - mat)*eg) / (MGTHRESH - EGTHRESH);
    }
    score = mg;

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