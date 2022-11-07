#include "see.h"

// Static exchange evaluation

const int simple_piece_values[13] = {10000,100,300,320,500,900,10000,100,300,320,500,900,-1};

// Mask of pieces that directly attack a given square
static inline U64 attacks_to(board_t* board, enumSquare target) {
    U64 occ = board->occupancies[BOTH];
    U64 attacks = (pawnAttackTable[WHITE][target] & board->bitboards[p]) | (pawnAttackTable[BLACK][target] & board->bitboards[P]);
    attacks |= knightAttackTable[target] & (board->bitboards[N] | board->bitboards[n]);
    attacks |= kingAttackTable[target] & (board->bitboards[k] | board->bitboards[K]);
    U64 bishops = (board->bitboards[b] | board->bitboards[B]) | (board->bitboards[q] | board->bitboards[Q]);
    U64 rooks = (board->bitboards[r] | board->bitboards[R]) | (board->bitboards[q] | board->bitboards[Q]);
    if (bishopAttackTable[target][0] & bishops)
        attacks |= (bishopAttacks(target, occ) & bishops);
    if (rookAttackTable[target][0] & rooks)
        attacks |= (rookAttacks(target, occ) & rooks);
    return attacks;
}

int SEE(board_t* board, enumSide side, U16 move) {
    U64 attacks, temp = C64(0), toccupied = board->occupancies[BOTH];
    U64 bsliders = (board->bitboards[b] | board->bitboards[B]) | (board->bitboards[q] | board->bitboards[Q]);
    U64 rsliders = (board->bitboards[r] | board->bitboards[R]) | (board->bitboards[q] | board->bitboards[Q]);
    int attacked_piece, piece, nc=1, see_list[32];
    int source = MOVE_FROM(move), target = MOVE_TO(move);

    attacks = attacks_to(board,target);
    attacked_piece = simple_piece_values[board->squares[target]];
    side ^= 1;

    see_list[0] = attacked_piece;
    toccupied &= ~((U64)1 << source);
    piece = board->squares[source];
    attacked_piece = simple_piece_values[piece];

    int piece_type = piece % 6;
    if (piece & 1) {attacks |= bishopAttacks(target, toccupied) & bsliders;} // Pawn, bishop, queen
    if (piece_type != K && (piece_type == P || piece_type > B)) {attacks |= rookAttacks(target, toccupied) & rsliders;}

    // Pick out least valuable attacker
    for (attacks &= toccupied; attacks; attacks &= toccupied) {
        for (piece = P; piece <= k; piece++) {
            if ((temp = board->bitboards[(piece % 6) + 6 * side] & attacks)) {
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
        attacked_piece = simple_piece_values[piece];
        if (see_list[nc++] - attacked_piece > 0)
            break;
        side ^= 1;    
    }
    while (--nc)
        see_list[nc - 1] = (-see_list[nc - 1] > see_list[nc]) ? see_list[nc - 1] : -see_list[nc];
    return see_list[0];
}