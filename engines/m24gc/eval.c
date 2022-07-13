#include "m24gc.h"
#include "uci.h"

// Piece values
int piece_values[6] = {20000, 100, 300, 325, 500, 900};

// Penalties and bonuses for positional factors
int DOUBLED_PAWN_PENALTY = 10;
int ISOLATED_PAWN_PENALTY = 20;
int BACKWARDS_PAWN_PENALTY = 10;
int PASSED_PAWN_BONUS = 25;
int ROOK_SEVENTH_BONUS = 30;
int ROOK_OPEN_FILE_BONUS = 20;
int ROOK_SEMI_OPEN_FILE_BONUS = 12;
int UNDEVELOPED_PIECE_PENALTY = 15;

// Piece-Square tables for positional evaluation
int pawn_pstable[64] = 
{0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, -30, -30, 0, 0, 0,
0, 0, 3, -10, -10, 3, 0, 0,
0, 2, 10, 10, 10, 3, 2, 0,
5, 4, 10, 12, 12, 10, 4, 5,
6, 6, 12, 16, 16, 12, 6, 6,
8, 8, 16, 20, 20, 16, 8, 8,
0, 0, 0, 0, 0, 0, 0, 0
};

int king_midgame_pstable[64] = 
{0, 20, 40, -15, 0, -15, 40, 20,
-10,-10, -20, -30, -30, -20, -10, -10,
-40, -40, -40, -50, -50, -40, -40, -40,
-40, -40, -40, -50, -50, -40, -40, -40,
-40, -40, -40, -50, -50, -40, -40, -40,
-40, -40, -40, -50, -50, -40, -40, -40,
-40, -40, -40, -50, -50, -40, -40, -40,
-40, -40, -40, -50, -50, -40, -40, -40
};

int king_endgame_pstable[64] = 
{-30, -15, 0, 15, 15, 0, -15, -30,
-15, 0, 15, 30, 30, 15, 0, -15,
0, 15, 30, 45, 45, 30, 15, 0,
15, 30, 45, 60, 60, 45, 30, 15,
15, 30, 45, 60, 60, 45, 30, 15,
0, 15, 30, 45, 45, 30, 15, 0,
-15, 0, 15, 30, 30, 15, 0, -15,
-30, -15, 0, 15, 15, 0, -15, -30
};


int knight_pstable[64] = 
{-20,-10,-5,-5,-5,-5,-10,-20,
-10, 0, 0, 3, 3, 0, 0,-10,
-5, 0, 5, 5, 5, 5, 0, -5,
-5, 0, 5, 10, 10 ,5 ,0,-5,
-5, 0, 5, 10, 10 ,5 ,0,-5,
-5, 0, 5, 5, 5, 5, 0, -5,
-10, 0, 0, 3, 3, 0, 0,-10,
-20,-10,-5,-5,-5,-5,-10,-20
};

int bishop_pstable[64] = 
{-10, -10, -10, -10, -10, -10, -10, -10,
-10, 0, 0, 0, 0, 0, 0, -10,
-10, 0, 5, 5, 5, 5, 0, -10,
-5, 5, 10, 15, 15, 10, 5, -5,
-5, 5, 10, 15, 15, 10, 5, -5,
-10, 0, 5, 5, 5, 5, 0, -10,
-10, 0, 0, 0, 0, 0, 0, -10,
-10, -10, -10, -10, -10, -10, -10, -10
};

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
static inline U64 fileFill(U64 bb) {
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

static inline int count_white_passed_pawns(U64 wpawns, U64 bpawns) {
    U64 black_pawn_fill = southFill(bpawns);
    U64 black_pawn_guard = southOne(eastOne(black_pawn_fill) | westOne(black_pawn_fill));
    U64 passed_pawns = wpawns & ~black_pawn_guard;
    return popcount(passed_pawns);
}

static inline int count_black_passed_pawns(U64 wpawns, U64 bpawns) {
    U64 white_pawn_fill = northFill(bpawns);
    U64 white_pawn_guard = northOne(eastOne(white_pawn_fill) | westOne(white_pawn_fill));
    U64 passed_pawns = bpawns & ~white_pawn_guard;
    return popcount(passed_pawns);
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

static inline int eval_king(int side, int opponent_material) {
    int score=0;
    U64 king_bb = bitboards[K + 6*side_to_move];
    int king_square = bitscanForward(king_bb);
    if (opponent_material >= 1400) {
        // Evaluate the positioning in the middlegame
        U64 pawn_bb = bitboards[P + 6*side_to_move];
        U64 our_side = side_to_move ? ~white_half : white_half;
        int score;
        if (king_bb && kingside) {
            pawn_bb &= kingside & our_side;
            score += (-120 + 40 * popcount(pawn_bb));
        } else if (king_bb && queenside) {
            pawn_bb &= queenside & our_side;
            score += (-120 + 40 * popcount(pawn_bb));
        } else {
            // Penalty for open files near the king
            U64 open_files = ~fileFill(bitboards[p] | bitboards[P]);
            score -= popcount(king_bb & open_files) * 40;
        }
        return score + king_midgame_pstable[(side_to_move ? FLIP(king_square) : king_square)];
    } else {
        // Endgame
        return king_endgame_pstable[king_square];
    }
}

// Evaluate the current position. If the posiiton is good for the side to move, returns a + score
int eval() {
    int white_score = 0, black_score = 0;
    int white_pieces=0, black_pieces=0;
    int white_pawns=0, black_pawns=0;
    U64 bitboard;
    int i, sq;
    //Material scores for rooks and queens
    white_pieces += popcount(bitboards[Q]) * 900;
    black_pieces += popcount(bitboards[q]) * 900;
    white_pieces += popcount(bitboards[R]) * 500;
    black_pieces += popcount(bitboards[r]) * 500;
    // Temporary measure
    if (bitboards[Q] && ((U64)1 << d1))
        white_score += 100;
    if (bitboards[q] && ((U64)1 << d8))
        black_score += 100;
    //Piece square tables and material scores for knights and bishops
    bitboard = bitboards[N];
    while (bitboard) {
        sq = bitscanForward(bitboard);
        clear_ls1b(bitboard);
        white_score += knight_pstable[sq];
        white_pieces += 300;
    }
    bitboard = bitboards[n];
    while (bitboard) {
        sq = bitscanForward(bitboard);
        clear_ls1b(bitboard);
        black_score += knight_pstable[sq];
        black_pieces += 300;
    }
    bitboard = bitboards[B];
    while (bitboard) {
        sq = bitscanForward(bitboard);
        clear_ls1b(bitboard);
        white_score += knight_pstable[sq];
        white_pieces += 330;
    }
    bitboard = bitboards[b];
    while (bitboard) {
        sq = bitscanForward(bitboard);
        clear_ls1b(bitboard);
        black_score += knight_pstable[sq];
        black_pieces += 330;
    }

    // Evaluate king activity and safety based on piece material
    white_score += eval_king(WHITE, black_pieces);
    black_score += eval_king(BLACK, white_pieces);

    // Piece square tables for pawns
    bitboard = bitboards[P];
    while (bitboard) {
        sq = bitscanForward(bitboard);
        clear_ls1b(bitboard);
        white_score += pawn_pstable[sq];
        white_pawns += 100;
    }
    bitboard = bitboards[p];
    while (bitboard) {
        sq = bitscanForward(bitboard);
        clear_ls1b(bitboard);
        black_score += pawn_pstable[FLIP(sq)];
        black_pawns += 100;
    }
    white_score += white_pieces + white_pawns;
    black_score += black_pieces + black_pawns;
    
    // Other positional evaluation considerations
    white_score += PASSED_PAWN_BONUS * count_white_passed_pawns(bitboards[P], bitboards[p]);
    black_score += PASSED_PAWN_BONUS * count_black_passed_pawns(bitboards[P], bitboards[p]);

    white_score -= ISOLATED_PAWN_PENALTY * count_isolated_pawns(bitboards[P]);
    black_score -= ISOLATED_PAWN_PENALTY * count_isolated_pawns(bitboards[p]);

    white_score -= DOUBLED_PAWN_PENALTY * count_doubled_pawns(bitboards[P]);
    black_score -= DOUBLED_PAWN_PENALTY * count_doubled_pawns(bitboards[p]);

    white_score -= BACKWARDS_PAWN_PENALTY * count_white_backward_pawns(bitboards[P]);
    black_score -= BACKWARDS_PAWN_PENALTY * count_black_backward_pawns(bitboards[p]);

    // Rook activity
    U64 open_files = ~fileFill(bitboards[p] | bitboards[P]);
    white_score += ROOK_OPEN_FILE_BONUS * popcount(bitboards[R] & open_files);
    black_score += ROOK_OPEN_FILE_BONUS * popcount(bitboards[r] & open_files);

    U64 semiopen_files = ~fileFill(bitboards[P]);
    white_score += ROOK_SEMI_OPEN_FILE_BONUS * popcount(bitboards[R] & semiopen_files & ~open_files);
    semiopen_files = ~fileFill(bitboards[p]);
    black_score += ROOK_SEMI_OPEN_FILE_BONUS * popcount(bitboards[r] & semiopen_files & ~open_files);

    white_score += ROOK_SEVENTH_BONUS * popcount(bitboards[R] & seventhRank);
    black_score += ROOK_SEVENTH_BONUS * popcount(bitboards[r] & secondRank);

    // Undeveloped penalty
    white_score -= popcount((bitboards[N] | bitboards[B]) & firstRank) * UNDEVELOPED_PIECE_PENALTY;
    black_score -= popcount((bitboards[n] | bitboards[b]) & lastRank) * UNDEVELOPED_PIECE_PENALTY; 

    if (side_to_move) 
        return black_score - white_score;
    else
        return white_score - black_score;

}

