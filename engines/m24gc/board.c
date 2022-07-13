#include "defs.h"
#include "data.h"
#include "protos.h"

void print_board() {
    printf("\n");
    for (int i=7; i>=0; i--) {
        printf("%d   ", i+1);
        for (int j=0; j<8;j++) {
            printf("%c ", piece_characters[piece_on_square[8*i + j]]);
        }
        printf("\n");
    }
    printf("\n    ");
    for (int j=0; j<8; j++)
        printf("%c ", 'a' + j);
    printf("\n\nBoard State:\n");
    printf("Side to move: %d\t", side_to_move);
    printf("Castling rights: %d\n", castling_rights);
    printf("En Passent allowed: %d\t", en_passent_legal);
    if (en_passent_legal) {
        printf("En Passent Target: %s\n", square_strings[en_passent_square]);
    }
    printf("\n");
}

void parse_fen(char *fen) {
    memset(&bitboards, 0, sizeof(bitboards));
    memset(&occupancies, 0, sizeof(occupancies));
    memset(&piece_on_square, EMPTY, sizeof(piece_on_square));
    en_passent_legal = 0;
    en_passent_square = 0;
    castling_rights = 0;
    side_to_move=0;

    for (int r=7; r >= 0; r--) {
        for (int f=0; f <= 8; f++) {
            if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z')) {
                char piece = char_to_piece_code[*fen++];
                bitboards[piece] |= ((U64)1 << (8*r + f));
                piece_on_square[8*r + f] = piece;
            }
            if (*fen >= '0' && *fen <= '9') {
                int offset = *fen++ - '0';
                if (piece_on_square[8*r + f] == EMPTY) f--;
                f += offset;
            }
            if (*fen == '/') {
                fen++;
                break;
            }
        }
    }
    /* Side to move */
    if (*(++fen) == 'b') side_to_move = BLACK;
    fen += 2;
    /* Castling rights */
    while (*fen != ' ') {
        switch(*fen) {
            case 'K': castling_rights |= WCK; break;
            case 'Q': castling_rights |= WCQ; break;
            case 'k': castling_rights |= BCK; break;
            case 'q': castling_rights |= BCQ; break;
        }
        fen++;
    }
    fen++;
    /* en passent square */
    if (*fen != '-') {
        int rk = *fen++ - 'a';
        int fl = *fen - '1';
        en_passent_square = 8 * rk + fl;
        en_passent_legal = 1;
    } else
        en_passent_legal = 0;

    /* occupancies */
    for (int i=0; i<6; i++) {
        occupancies[WHITE] |= bitboards[i];
        occupancies[BLACK] |= bitboards[6 + i];
    }
    occupancies[BOTH] = occupancies[WHITE] | occupancies[BLACK];    
}
 
/* =================================
           Move Generation
 =================================== */

int is_square_attacked(int sq, int side) {
    if (pawn_attack_table[1^side][sq] & bitboards[6*side + 1]) return _TRUE;
    if (king_attack_table[sq] & bitboards[6*side]) return _TRUE;
    if (knight_attack_table[sq] & bitboards[6*side + 2]) return _TRUE;
    if (rookAttacks(sq, occupancies[BOTH]) & (bitboards[6*side + 4] | bitboards[6*side + 5])) return _TRUE;
    if (bishopAttacks(sq, occupancies[BOTH]) & (bitboards[6*side + 3] | bitboards[6*side + 5])) return _TRUE;
    return _FALSE;
}

/* Returns whether the king is in check */
int in_check(int side) {
    int king = bitscanForward(bitboards[6*side]);
    return is_square_attacked(king, 1 ^ side);
}

U64 all_checkers() {
    U64 checkers = 0;
    int king = bitscanForward(bitboards[6*side_to_move]);
    checkers |= (pawn_attack_table[side_to_move][king] & bitboards[p - 6*side_to_move]);
    checkers |= (king_attack_table[king] & bitboards[k - 6*side_to_move]);
    checkers |= (knight_attack_table[king] & bitboards[n - 6*side_to_move]);
    checkers |= ((rookAttacks(king, occupancies[BOTH]) & (bitboards[r - 6*side_to_move] | bitboards[q - 6*side_to_move])));
    checkers |= ((bishopAttacks(king, occupancies[BOTH]) & (bitboards[b - 6*side_to_move] | bitboards[q - 6*side_to_move])));
    return checkers;
}

/* Helper functions */
void print_move(U16 move_data) {
    if ((move_flags(move_data)) & 0x8) {
        printf("%s%s%c", square_strings[move_source(move_data)], square_strings[move_target(move_data)], promoted_pieces[(move_flags(move_data)) & 0x3]);
    } else {
        printf("%s%s", square_strings[move_source(move_data)], square_strings[move_target(move_data)]);
    }
}

void print_move_stack(int ply) {
    printf("Moves at ply %d\n", ply);
    for (int i=moves_start_idx[ply]; i<moves_start_idx[ply+1]; i++) {
        print_move(move_stack.moves[i].move);
        printf("\n");
    }
}

void print_move_history(int ply) {
    printf("Moves to ply %d\n", ply);
    for (int i=0; i<ply; i++) {
        print_move(game_history[i].move);
        printf("\n");
    }
}

void add_move(U16 data) {
    move_stack.moves[moves_start_idx[ply+1]++].move = data;
}

// Constants for pawn promotion
static const U64 secondRank = C64(0xFF00);
static const U64 seventhRank = C64(0xFF000000000000);

/* Capture generator */
void generate_captures() {
    U64 occ_both = occupancies[BOTH];
    U64 occ_white = occupancies[WHITE];
    U64 occ_black = occupancies[BLACK];
    moves_start_idx[ply+1] = moves_start_idx[ply];
    int source, target;
    U64 bitboard, attacks;
    if (side_to_move == WHITE) {
        /* King */
        source = bitscanForward(bitboards[K]);
        attacks = king_attack_table[source] & occ_black;
        while (attacks) {
            target = bitscanForward(attacks);
            clear_ls1b(attacks);
            add_move(encode_move(source, target, 4));
        }
        /* Pawns */
        /* Pawn pushes and captures */
        bitboard = bitboards[P] & ~seventhRank;
        while (bitboard) {
            source = bitscanForward(bitboard);
            clear_ls1b(bitboard);
            /* Captures */ 
            attacks = pawn_attack_table[WHITE][source] & occ_black;
            while (attacks) {
                target = bitscanForward(attacks);
                clear_ls1b(attacks);
                add_move(encode_move(source, target, 4));
            }
        }
        /* Promotions */
        bitboard = bitboards[P] & seventhRank;
        while (bitboard) {
            source = bitscanForward(bitboard);
            clear_ls1b(bitboard);
            /* Capture Promotions */ 
            attacks = pawn_attack_table[WHITE][source] & occ_black;
            while (attacks) {
                target = bitscanForward(attacks);
                clear_ls1b(attacks);
                add_move(encode_move(source, target, 12));
                add_move(encode_move(source, target, 13));
                add_move(encode_move(source, target, 14));
                add_move(encode_move(source, target, 15));
            }
        }
        /* Special (Castling, En Passent) */
        /* EP */
        if (en_passent_legal) {
            bitboard = pawn_attack_table[BLACK][en_passent_square] & bitboards[P];
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                add_move(encode_move(source, en_passent_square, 5));
            }
        }
        /* Knights */
        bitboard = bitboards[N];
        while (bitboard) {
            source = bitscanForward(bitboard);
            clear_ls1b(bitboard);
            attacks = knight_attack_table[source] & occ_black;
            while (attacks) {
                target = bitscanForward(attacks);
                clear_ls1b(attacks);
                add_move(encode_move(source, target, 4));
            }
        }
        /* Bishops */
        bitboard = bitboards[B] | bitboards[Q];
        while (bitboard) {
            source = bitscanForward(bitboard);
            clear_ls1b(bitboard);
            attacks = bishopAttacks(source, occ_both) & occ_black;
            while (attacks) {
                target = bitscanForward(attacks);
                clear_ls1b(attacks);
                add_move(encode_move(source, target, 4));
            }
        }
        /* Rooks */
        bitboard = bitboards[R] | bitboards[Q];
        while (bitboard) {
            source = bitscanForward(bitboard);
            clear_ls1b(bitboard);
            attacks = rookAttacks(source, occ_both) & occ_black;
            while (attacks) {
                target = bitscanForward(attacks);
                clear_ls1b(attacks);
                add_move(encode_move(source, target, 4));
            }
        }
    } else {
        /* King */
        source = bitscanForward(bitboards[k]);
        attacks = king_attack_table[source] & occ_white;
        while (attacks) {
            target = bitscanForward(attacks);
            clear_ls1b(attacks);
            add_move(encode_move(source, target, 4));
        }
        /* Pawns */
        /* Pawn pushes and captures */
        bitboard = bitboards[p] & ~secondRank;
        while (bitboard) {
            source = bitscanForward(bitboard);
            clear_ls1b(bitboard);
            /* Captures */ 
            attacks = pawn_attack_table[BLACK][source] & occ_white;
            while (attacks) {
                target = bitscanForward(attacks);
                clear_ls1b(attacks);
                add_move(encode_move(source, target, 4));
            }
        }
        /* Promotions */
        bitboard = bitboards[p] & secondRank;
        while (bitboard) {
            source = bitscanForward(bitboard);
            clear_ls1b(bitboard);
            /* Capture Promotions */ 
            attacks = pawn_attack_table[BLACK][source] & occ_white;
            while (attacks) {
                target = bitscanForward(attacks);
                clear_ls1b(attacks);
                add_move(encode_move(source, target, 12));
                add_move(encode_move(source, target, 13));
                add_move(encode_move(source, target, 14));
                add_move(encode_move(source, target, 15));
            }
        }
        /* Special (Castling, En Passent) */
        /* EP */
        if (en_passent_legal) {
            bitboard = pawn_attack_table[WHITE][en_passent_square] & bitboards[p];
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                add_move(encode_move(source, en_passent_square, 5));
            }
        }
        /* Knights */
        bitboard = bitboards[n];
        while (bitboard) {
            source = bitscanForward(bitboard);
            clear_ls1b(bitboard);
            attacks = knight_attack_table[source] & occ_white;
            while (attacks) {
                target = bitscanForward(attacks);
                clear_ls1b(attacks);
                add_move(encode_move(source, target, 4));
            }
        }
        /* Bishops */
        bitboard = bitboards[b] | bitboards[q];
        while (bitboard) {
            source = bitscanForward(bitboard);
            clear_ls1b(bitboard);
            attacks = bishopAttacks(source, occ_both) & occ_white;
            while (attacks) {
                target = bitscanForward(attacks);
                clear_ls1b(attacks);
                add_move(encode_move(source, target, 4));
            }
        }
        /* Rooks */
        bitboard = bitboards[r] | bitboards[q];
        while (bitboard) {
            source = bitscanForward(bitboard);
            clear_ls1b(bitboard);
            attacks = rookAttacks(source, occ_both) & occ_white;
            while (attacks) {
                target = bitscanForward(attacks);
                clear_ls1b(attacks);
                add_move(encode_move(source, target, 4));
            }
        }
    }
    move_stack.count = moves_start_idx[ply+1];
}

U64 diagonal_pinmask() {
    int kingsq = bitscanForward(bitboards[K + 6*side_to_move]);
    U64 beyond_first_blockers = bishopPinAttacks(kingsq, occupancies[side_to_move]) & bishopPinAttacks(kingsq, occupancies[BOTH]);
    U64 pinners = beyond_first_blockers & (bitboards[b - 6*side_to_move] | bitboards[q - 6*side_to_move]);
    U64 mask = 0;
    U64 pinners_copy = pinners;
    while (pinners) {
        int b = bitscanForward(pinners);
        clear_ls1b(pinners);
        mask |= bishopAttacks(b, (U64)1 << kingsq);
    }
    return pinners_copy | (bishopAttacks(kingsq, pinners_copy) & mask);
}

U64 rankfile_pinmask() {
    int kingsq = bitscanForward(bitboards[K + 6*side_to_move]);
    U64 beyond_first_blockers = rookPinAttacks(kingsq, occupancies[side_to_move]) & rookPinAttacks(kingsq, occupancies[BOTH]);;
    U64 pinners = beyond_first_blockers & (bitboards[r - 6*side_to_move] | bitboards[q - 6*side_to_move]);
    U64 mask = 0;
    U64 pinners_copy = pinners;
    while (pinners) {
        int b = bitscanForward(pinners);
        clear_ls1b(pinners);
        mask |= rookAttacks(b, (U64)1 << kingsq);
    }
    return pinners_copy | (rookAttacks(kingsq, pinners_copy) & mask);
}

void generate_check(U64 checkers) {
    U64 bitboard;
    int target;
    U64 occ_black = occupancies[BLACK];
    U64 occ_white = occupancies[WHITE];
    U64 occ_both = occupancies[BOTH];
    if (checkers & (checkers - 1)) {
        // Double check
        int source = bitscanForward(bitboards[K + 6*side_to_move]);
        U64 attacks = king_attack_table[source] & ~(side_to_move ? occupancies[BLACK] : occupancies[WHITE]);
        while (attacks) {
            int target = bitscanForward(attacks);
            clear_ls1b(attacks);
            U64 from_to_bb = ((U64)1 << source) | ((U64)1 << target);
            // inline lightweight make move
            bitboards[K + 6*side_to_move] ^= from_to_bb;
            occupancies[BOTH] ^= from_to_bb;
            // If not left in check, this is a legal move
            if (!is_square_attacked(target, 1 ^ side_to_move)) {
                add_move(encode_move(source, target, (piece_on_square[target] == EMPTY ? 0 : 4) ));
            }
            // inline lightweight unmake move
            bitboards[K + 6*side_to_move] ^= from_to_bb;
            occupancies[BOTH] ^= from_to_bb;
        }
        return;
    } else {
        // Only in a single check
        U64 checkmask;
        int checksq = bitscanForward(checkers);
        int kingsq = bitscanForward(bitboards[K + 6*side_to_move]);
        if ((piece_on_square[checksq] % 6) > N) {
            if (bishopAttackTable[checksq][0] & ((U64)1 << kingsq))
                checkmask = checkers | (bishopAttacks(kingsq, occupancies[BOTH]) & bishopAttacks(checksq, occupancies[BOTH]));
            else
                checkmask = checkers | (rookAttacks(kingsq, occupancies[BOTH]) & rookAttacks(checksq, occupancies[BOTH]));

        } else
            checkmask = checkers;
        // King moves
        int source = bitscanForward(bitboards[K + 6*side_to_move]);
        U64 attacks = king_attack_table[source] & ~(side_to_move ? occupancies[BLACK] : occupancies[WHITE]);
        while (attacks) {
            int target = bitscanForward(attacks);
            clear_ls1b(attacks);
            U64 from_to_bb = ((U64)1 << source) | ((U64)1 << target);
            // inline lightweight make move
            bitboards[K + 6*side_to_move] ^= from_to_bb;
            occupancies[BOTH] ^= from_to_bb;
            // If not left in check, this is a legal move
            if (!is_square_attacked(target, 1 ^ side_to_move)) {
                add_move(encode_move(source, target, (piece_on_square[target] == EMPTY ? 0 : 4) ));
            }
            // inline lightweight unmake move
            bitboards[K + 6*side_to_move] ^= from_to_bb;
            occupancies[BOTH] ^= from_to_bb;
        }
        U64 rook_pinmask = rankfile_pinmask();
        U64 bishop_pinmask = diagonal_pinmask();
        U64 pawns;
        // No castling.
        // Rest of the moves must intersect the checkmask.
        if (side_to_move == WHITE) {
            /* Pawns */
            /* Pawn pushes and captures */
            bitboard = bitboards[P] & ~seventhRank;
            pawns = bitboard & bishop_pinmask;
            // If being pinned by a bishop, can only move along pin line and check mask
            while (pawns) {
                /* Captures */ 
                source = bitscanForward(pawns);
                clear_ls1b(pawns); 
                attacks = pawn_attack_table[WHITE][source] & occ_black & bishop_pinmask & checkmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 4));
                }
            }
            // If it's pinned by a rook, no way to step up and block a check

            // If pinned by neither, can either step up to block or capture the checking piece
            bitboard &= (~rook_pinmask & ~bishop_pinmask);
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                /* single pushes */
                if (piece_on_square[source + 8] == EMPTY) {
                    if (checkmask & ((U64)1 << (source + 8))) {
                        add_move(encode_move(source, source + 8, 0));
                    }
                    /* Double pushes */
                    if ((source < 16) && piece_on_square[source + 16] == EMPTY) {
                        if (checkmask & ((U64)1 << (source + 16)))
                            add_move(encode_move(source, source + 16, 1)); 
                    }
                }
                attacks = pawn_attack_table[WHITE][source] & occ_black & checkmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 4));
                }
            }
            /* Promotions */
            bitboard = bitboards[P] & seventhRank;
            // Again, can't step up if pinned by the rook
            pawns = bitboard & bishop_pinmask;
            while (pawns) {
                /* Capture Promotions */
                source = bitscanForward(pawns);
                clear_ls1b(pawns); 
                attacks = pawn_attack_table[WHITE][source] & occ_black & bishop_pinmask & checkmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 12));
                    add_move(encode_move(source, target, 13));
                    add_move(encode_move(source, target, 14));
                    add_move(encode_move(source, target, 15));
                }
            }
            bitboard &= (~bishop_pinmask & ~rook_pinmask);
            while (bitboard) {
                /* Capture Promotions */
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                if (piece_on_square[source + 8] == EMPTY) {
                    if (checkmask & ((U64)1 << (source + 8))) {
                    add_move(encode_move(source, source + 8, 8));
                    add_move(encode_move(source, source + 8, 9));
                    add_move(encode_move(source, source + 8, 10));
                    add_move(encode_move(source, source + 8, 11));
                    }
                } 
                attacks = pawn_attack_table[WHITE][source] & occ_black & checkmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 12));
                    add_move(encode_move(source, target, 13));
                    add_move(encode_move(source, target, 14));
                    add_move(encode_move(source, target, 15));
                }
            }
            /* En Passent */
            if (en_passent_legal) {
                int target = en_passent_square;
                bitboard = pawn_attack_table[BLACK][en_passent_square] & bitboards[P] & checkmask;
                while (bitboard) {
                    source = bitscanForward(bitboard);
                    clear_ls1b(bitboard);
                    U64 from_to_bb = ((U64)1 << source) | ((U64)1 << target);
                    // inline lightweight make move
                    bitboards[P] ^= from_to_bb;
                    bitboards[p] ^= ((U64)1 << (target - 8));
                    occupancies[BOTH] ^= (from_to_bb | ((U64)1 << (target - 8)));
                    // If not left in check, this is a legal move
                    if (!is_square_attacked(kingsq, BLACK)) {
                        add_move(encode_move(source, target,5));
                    }
                    // inline lightweight unmake move
                    bitboards[P] ^= from_to_bb;
                    bitboards[p] ^= ((U64)1 << (target - 8));
                    occupancies[BOTH] ^= (from_to_bb | ((U64)1 << (target - 8)));
                }
            }
            /* Knights */
            bitboard = bitboards[N] & ~bishop_pinmask & ~rook_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = knight_attack_table[source] & ~occ_white & checkmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            /* Bishops (+ queens) */
            bitboard = (bitboards[B] | bitboards[Q]) & ~rook_pinmask & bishop_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = bishopAttacks(source, occ_both) & ~occ_white & checkmask & bishop_pinmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            bitboard = (bitboards[B] | bitboards[Q]) & ~rook_pinmask & ~bishop_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = bishopAttacks(source, occ_both) & ~occ_white & checkmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            /* Rooks (+ queens) */
            bitboard = (bitboards[R] | bitboards[Q]) & ~bishop_pinmask & rook_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = rookAttacks(source, occ_both) & ~occ_white & checkmask & rook_pinmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            bitboard = (bitboards[R] | bitboards[Q]) & ~bishop_pinmask & ~rook_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = rookAttacks(source, occ_both) & ~occ_white & checkmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
        } else {
            /* Pawns */
            /* Pawn pushes and captures */
            bitboard = bitboards[p] & ~secondRank;
            pawns = bitboard & bishop_pinmask;
            // If being pinned by a bishop, can only move along pin line and check mask
            while (pawns) {
                /* Captures */ 
                source = bitscanForward(pawns);
                clear_ls1b(pawns); 
                attacks = pawn_attack_table[BLACK][source] & occ_white & bishop_pinmask & checkmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 4));
                }
            }
            // If it's pinned by a rook, no way to step up and block a check

            // If pinned by neither, can either step up to block or capture the checking piece
            bitboard &= (~rook_pinmask & ~bishop_pinmask);
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                /* single pushes */
                if (piece_on_square[source - 8] == EMPTY) {
                    if (checkmask & ((U64)1 << (source - 8)))
                        add_move(encode_move(source, source - 8, 0));
                    /* Double pushes */
                    if ((source >= 48) && piece_on_square[source - 16] == EMPTY) {
                        if (checkmask & ((U64)1 << (source - 16)))
                            add_move(encode_move(source, source - 16, 1)); 
                    }
                }
                attacks = pawn_attack_table[BLACK][source] & occ_white & checkmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 4));
                }
            }
            /* Promotions */
            bitboard = bitboards[p] & secondRank;
            // Again, can't step up if pinned by the rook
            pawns = bitboard & bishop_pinmask;
            while (pawns) {
                /* Capture Promotions */
                source = bitscanForward(pawns);
                clear_ls1b(pawns); 
                attacks = pawn_attack_table[BLACK][source] & occ_white & bishop_pinmask & checkmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 12));
                    add_move(encode_move(source, target, 13));
                    add_move(encode_move(source, target, 14));
                    add_move(encode_move(source, target, 15));
                }
            }
            bitboard &= (~bishop_pinmask & ~rook_pinmask);
            while (bitboard) {
                /* Capture Promotions */
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                if (piece_on_square[source - 8] == EMPTY) {
                    if (checkmask & ((U64)1 << (source - 8))) {
                    add_move(encode_move(source, source - 8, 8));
                    add_move(encode_move(source, source - 8, 9));
                    add_move(encode_move(source, source - 8, 10));
                    add_move(encode_move(source, source - 8, 11));
                    }
                } 
                attacks = pawn_attack_table[BLACK][source] & occ_white & checkmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 12));
                    add_move(encode_move(source, target, 13));
                    add_move(encode_move(source, target, 14));
                    add_move(encode_move(source, target, 15));
                }
            }
            /* En Passent */
            if (en_passent_legal) {
                int target = en_passent_square;
                bitboard = pawn_attack_table[WHITE][en_passent_square] & bitboards[p] & checkmask;
                while (bitboard) {
                    source = bitscanForward(bitboard);
                    clear_ls1b(bitboard);
                    U64 from_to_bb = ((U64)1 << source) | ((U64)1 << target);
                    // inline lightweight make move
                    bitboards[p] ^= from_to_bb;
                    bitboards[P] ^= ((U64)1 << (target + 8));
                    occupancies[BOTH] ^= (from_to_bb | ((U64)1 << (target + 8)));
                    // If not left in check, this is a legal move
                    if (!is_square_attacked(kingsq, WHITE)) {
                        add_move(encode_move(source, target,5));
                    }
                    // inline lightweight unmake move
                    bitboards[p] ^= from_to_bb;
                    bitboards[P] ^= ((U64)1 << (target + 8));
                    occupancies[BOTH] ^= (from_to_bb | ((U64)1 << (target + 8)));
                }
            }
            /* Knights */
            bitboard = bitboards[n] & ~bishop_pinmask & ~rook_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = knight_attack_table[source] & ~occ_black & checkmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            /* Bishops (+ queens) */
            bitboard = (bitboards[b] | bitboards[q]) & ~rook_pinmask & bishop_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = bishopAttacks(source, occ_both) & ~occ_black & checkmask & bishop_pinmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            bitboard = (bitboards[b] | bitboards[q]) & ~rook_pinmask & ~bishop_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = bishopAttacks(source, occ_both) & ~occ_black & checkmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            /* Rooks (+ queens) */
            bitboard = (bitboards[r] | bitboards[q]) & ~bishop_pinmask & rook_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = rookAttacks(source, occ_both) & ~occ_black & checkmask & rook_pinmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            bitboard = (bitboards[r] | bitboards[q]) & ~bishop_pinmask & ~rook_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = rookAttacks(source, occ_both) & ~occ_black & checkmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
        }
    }
}
/* Legal moves */
void generate_moves() {
    U64 occ_both = occupancies[BOTH];
    U64 occ_white = occupancies[WHITE];
    U64 occ_black = occupancies[BLACK];
    moves_start_idx[ply+1] = moves_start_idx[ply];
    int source, target;
    U64 bitboard, attacks, pawns;
    U64 checkers = all_checkers();
    if (checkers) {
        generate_check(checkers);
        move_stack.count = moves_start_idx[ply+1];
        return;
    }
    U64 rook_pinmask = rankfile_pinmask();
    U64 bishop_pinmask = diagonal_pinmask();
    if (side_to_move == WHITE) {
        // King
            int source = bitscanForward(bitboards[K]);
            attacks = king_attack_table[source] & ~occupancies[WHITE];
            while (attacks) {
                int target = bitscanForward(attacks);
                clear_ls1b(attacks);
                U64 from_to_bb = ((U64)1 << source) | ((U64)1 << target);
                // inline lightweight make move
                bitboards[K] ^= from_to_bb;
                occupancies[BOTH] ^= from_to_bb;
                // If not left in check, this is a legal move
                if (!is_square_attacked(target, BLACK)) {
                    add_move(encode_move(source, target, (piece_on_square[target] == EMPTY ? 0 : 4) ));
                }
                // inline lightweight unmake move
                bitboards[K] ^= from_to_bb;
                occupancies[BOTH] ^= from_to_bb;
            }
            /* Pawns */
            /* Pawn pushes and captures */
            bitboard = bitboards[P] & ~seventhRank;
            pawns = bitboard & bishop_pinmask;
            while (pawns) {
                /* Captures */ 
                source = bitscanForward(pawns);
                clear_ls1b(pawns); 
                attacks = pawn_attack_table[WHITE][source] & occ_black & bishop_pinmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 4));
                }
            }
            pawns = bitboard & rook_pinmask;
            while (pawns) {
                /* Single pushes */
                source = bitscanForward(pawns);
                clear_ls1b(pawns);
                /* single pushes */
                if (piece_on_square[source + 8] == EMPTY) {
                    if (rook_pinmask & ((U64)1 << (source + 8))) {
                    add_move(encode_move(source, source + 8, 0));
                    /* Double pushes */
                    if ((source < 16) && piece_on_square[source + 16] == EMPTY)
                        add_move(encode_move(source, source + 16, 1)); 
                    }
                }
            }
            bitboard &= (~rook_pinmask & ~bishop_pinmask);
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                /* single pushes */
                if (piece_on_square[source + 8] == EMPTY) {
                    add_move(encode_move(source, source + 8, 0));
                    /* Double pushes */
                    if ((source < 16) && piece_on_square[source + 16] == EMPTY)
                        add_move(encode_move(source, source + 16, 1)); 
                }
                attacks = pawn_attack_table[WHITE][source] & occ_black;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 4));
                }
            }
            /* Promotions */
            bitboard = bitboards[P] & seventhRank;
            pawns = bitboard & rook_pinmask;
            while (pawns) {
                source = bitscanForward(pawns);
                clear_ls1b(pawns);
                /* Push promotions */
                if (piece_on_square[source + 8] == EMPTY) {
                    if (rook_pinmask & ((U64)1 << (source + 8))) {
                    add_move(encode_move(source, source + 8, 8));
                    add_move(encode_move(source, source + 8, 9));
                    add_move(encode_move(source, source + 8, 10));
                    add_move(encode_move(source, source + 8, 11));
                    }
                }
            }
            pawns = bitboard & bishop_pinmask;
            while (pawns) {
                /* Capture Promotions */
                source = bitscanForward(pawns);
                clear_ls1b(pawns); 
                attacks = pawn_attack_table[WHITE][source] & occ_black & bishop_pinmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 12));
                    add_move(encode_move(source, target, 13));
                    add_move(encode_move(source, target, 14));
                    add_move(encode_move(source, target, 15));
                }
            }
            bitboard &= (~bishop_pinmask & ~rook_pinmask);
            while (bitboard) {
                /* Capture Promotions */
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                if (piece_on_square[source + 8] == EMPTY) {
                    add_move(encode_move(source, source + 8, 8));
                    add_move(encode_move(source, source + 8, 9));
                    add_move(encode_move(source, source + 8, 10));
                    add_move(encode_move(source, source + 8, 11));
                } 
                attacks = pawn_attack_table[WHITE][source] & occ_black;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 12));
                    add_move(encode_move(source, target, 13));
                    add_move(encode_move(source, target, 14));
                    add_move(encode_move(source, target, 15));
                }
            }
            /* En Passent */
            if (en_passent_legal) {
                int target = en_passent_square;
                bitboard = pawn_attack_table[BLACK][en_passent_square] & bitboards[P];
                while (bitboard) {
                    source = bitscanForward(bitboard);
                    clear_ls1b(bitboard);
                    U64 from_to_bb = ((U64)1 << source) | ((U64)1 << target);
                    // inline lightweight make move
                    bitboards[P] ^= from_to_bb;
                    bitboards[p] ^= ((U64)1 << (target - 8));
                    occupancies[BOTH] ^= (from_to_bb | ((U64)1 << (target - 8)));
                    // If not left in check, this is a legal move
                    if (!is_square_attacked(bitscanForward(bitboards[K]), BLACK)) {
                        add_move(encode_move(source, target,5));
                    }
                    // inline lightweight unmake move
                    bitboards[P] ^= from_to_bb;
                    bitboards[p] ^= ((U64)1 << (target - 8));
                    occupancies[BOTH] ^= (from_to_bb | ((U64)1 << (target - 8)));
                }
            }

            /* Castling */
        /* Kingside */
        if ((castling_rights & WCK) && (piece_on_square[f1] == EMPTY) && (piece_on_square[g1] == EMPTY) && (!is_square_attacked(f1,BLACK)) && (!is_square_attacked(g1,BLACK))) {
            add_move(encode_move(e1, g1, 2));
        }
        /* Queenside */
        if ((castling_rights & WCQ) && (piece_on_square[d1] == EMPTY) && (piece_on_square[c1] == EMPTY) && (piece_on_square[b1] == EMPTY) && (!is_square_attacked(c1,BLACK)) && (!is_square_attacked(d1,BLACK))) {
            add_move(encode_move(e1, c1, 3));
        }

            /* Knights */
            bitboard = bitboards[N] & ~bishop_pinmask & ~rook_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = knight_attack_table[source] & ~occ_white;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            /* Bishops (+ queens) */
            bitboard = (bitboards[B] | bitboards[Q]) & ~rook_pinmask & bishop_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = bishopAttacks(source, occ_both) & ~occ_white & bishop_pinmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            bitboard = (bitboards[B] | bitboards[Q]) & ~rook_pinmask & ~bishop_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = bishopAttacks(source, occ_both) & ~occ_white;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            /* Rooks (+ queens) */
            bitboard = (bitboards[R] | bitboards[Q]) & ~bishop_pinmask & rook_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = rookAttacks(source, occ_both) & ~occ_white & rook_pinmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            bitboard = (bitboards[R] | bitboards[Q]) & ~bishop_pinmask & ~rook_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = rookAttacks(source, occ_both) & ~occ_white;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
        } else {
            // King
            int source = bitscanForward(bitboards[k]);
            attacks = king_attack_table[source] & ~occupancies[BLACK];
            while (attacks) {
                int target = bitscanForward(attacks);
                clear_ls1b(attacks);
                U64 from_to_bb = ((U64)1 << source) | ((U64)1 << target);
                // inline lightweight make move
                bitboards[k] ^= from_to_bb;
                occupancies[BOTH] ^= from_to_bb;
                // If not left in check, this is a legal move
                if (!is_square_attacked(target, WHITE)) {
                    add_move(encode_move(source, target, (piece_on_square[target] == EMPTY ? 0 : 4) ));
                }
                // inline lightweight unmake move
                bitboards[k] ^= from_to_bb;
                occupancies[BOTH] ^= from_to_bb;
            }
            /* Pawns */
            /* Pawn pushes and captures */
            bitboard = bitboards[p] & ~secondRank;
            pawns = bitboard & bishop_pinmask;
            while (pawns) {
                /* Captures */ 
                source = bitscanForward(pawns);
                clear_ls1b(pawns); 
                attacks = pawn_attack_table[BLACK][source] & occ_white & bishop_pinmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 4));
                }
            }
            pawns = bitboard & rook_pinmask;
            while (pawns) {
                /* Single pushes */
                source = bitscanForward(pawns);
                clear_ls1b(pawns);
                /* single pushes */
                if (piece_on_square[source - 8] == EMPTY) {
                    if (rook_pinmask & ((U64)1 << (source - 8))) {
                    add_move(encode_move(source, source - 8, 0));
                    /* Double pushes */
                    if ((source >= 48) && piece_on_square[source - 16] == EMPTY)
                        add_move(encode_move(source, source - 16, 1)); 
                    }
                }
            }
            bitboard &= (~rook_pinmask & ~bishop_pinmask);
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                /* single pushes */
                if (piece_on_square[source - 8] == EMPTY) {
                    add_move(encode_move(source, source - 8, 0));
                    /* Double pushes */
                    if ((source >= 48) && piece_on_square[source - 16] == EMPTY)
                        add_move(encode_move(source, source - 16, 1)); 
                }
                attacks = pawn_attack_table[BLACK][source] & occ_white;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 4));
                }
            }
            /* Promotions */
            bitboard = bitboards[p] & secondRank;
            pawns = bitboard & rook_pinmask;
            while (pawns) {
                source = bitscanForward(pawns);
                clear_ls1b(pawns);
                /* Push promotions */
                if (piece_on_square[source - 8] == EMPTY) {
                    if (rook_pinmask & ((U64)1 << (source - 8))) {
                    add_move(encode_move(source, source - 8, 8));
                    add_move(encode_move(source, source - 8, 9));
                    add_move(encode_move(source, source - 8, 10));
                    add_move(encode_move(source, source - 8, 11));
                    }
                }
            }
            pawns = bitboard & bishop_pinmask;
            while (pawns) {
                /* Capture Promotions */
                source = bitscanForward(pawns);
                clear_ls1b(pawns); 
                attacks = pawn_attack_table[BLACK][source] & occ_white & bishop_pinmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 12));
                    add_move(encode_move(source, target, 13));
                    add_move(encode_move(source, target, 14));
                    add_move(encode_move(source, target, 15));
                }
            }
            bitboard &= (~bishop_pinmask & ~rook_pinmask);
            while (bitboard) {
                /* Capture Promotions */
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                if (piece_on_square[source - 8] == EMPTY) {
                    add_move(encode_move(source, source - 8, 8));
                    add_move(encode_move(source, source - 8, 9));
                    add_move(encode_move(source, source - 8, 10));
                    add_move(encode_move(source, source - 8, 11));
                } 
                attacks = pawn_attack_table[BLACK][source] & occ_white;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, 12));
                    add_move(encode_move(source, target, 13));
                    add_move(encode_move(source, target, 14));
                    add_move(encode_move(source, target, 15));
                }
            }
            /* En Passent */
            if (en_passent_legal) {
                int target = en_passent_square;
                bitboard = pawn_attack_table[WHITE][en_passent_square] & bitboards[p];
                while (bitboard) {
                    source = bitscanForward(bitboard);
                    clear_ls1b(bitboard);
                    U64 from_to_bb = ((U64)1 << source) | ((U64)1 << target);
                    // inline lightweight make move
                    bitboards[p] ^= from_to_bb;
                    bitboards[P] ^= ((U64)1 << (target + 8));
                    occupancies[BOTH] ^= (from_to_bb | ((U64)1 << (target + 8)));
                    // If not left in check, this is a legal move
                    if (!is_square_attacked(bitscanForward(bitboards[k]), WHITE)) {
                        add_move(encode_move(source, target,5));
                    }
                    // inline lightweight unmake move
                    bitboards[p] ^= from_to_bb;
                    bitboards[P] ^= ((U64)1 << (target + 8));
                    occupancies[BOTH] ^= (from_to_bb | ((U64)1 << (target + 8)));
                }
            }
                    /* Castling */
        /* Kingside */
        if ((castling_rights & BCK) && (piece_on_square[f8] == EMPTY) && (piece_on_square[g8] == EMPTY) && (!is_square_attacked(f8,WHITE)) && (!is_square_attacked(g8,WHITE))) {
            add_move(encode_move(e8, g8, 2));
        }
        /* Queenside */
        if ((castling_rights & BCQ) && (piece_on_square[d8] == EMPTY) && (piece_on_square[c8] == EMPTY) && (piece_on_square[b8] == EMPTY)&& (!is_square_attacked(d8,WHITE)) && (!is_square_attacked(c8,WHITE))) {
            add_move(encode_move(e8, c8, 3));
        }
            /* Knights */
            bitboard = bitboards[n] & ~bishop_pinmask & ~rook_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = knight_attack_table[source] & ~occ_black;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            /* Bishops (+ queens) */
            bitboard = (bitboards[b] | bitboards[q]) & ~rook_pinmask & bishop_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = bishopAttacks(source, occ_both) & ~occ_black & bishop_pinmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            bitboard = (bitboards[b] | bitboards[q]) & ~rook_pinmask & ~bishop_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = bishopAttacks(source, occ_both) & ~occ_black;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            /* Rooks (+ queens) */
            bitboard = (bitboards[r] | bitboards[q]) & ~bishop_pinmask & rook_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = rookAttacks(source, occ_both) & ~occ_black & rook_pinmask;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
            bitboard = (bitboards[r] | bitboards[q]) & ~bishop_pinmask & ~rook_pinmask;
            while (bitboard) {
                source = bitscanForward(bitboard);
                clear_ls1b(bitboard);
                attacks = rookAttacks(source, occ_both) & ~occ_black;
                while (attacks) {
                    target = bitscanForward(attacks);
                    clear_ls1b(attacks);
                    add_move(encode_move(source, target, (piece_on_square[target]==EMPTY ? 0 : 4)));
                }
            }
        }
    move_stack.count = moves_start_idx[ply+1];
}

// Castling update masks for make move
const int castling_rights_update[64] = {
    13,15,15,15,12,15,15,14,
    15, 15,15,15,15,15,15,15,
    15, 15,15,15,15,15,15,15,
    15, 15,15,15,15,15,15,15,
    15, 15,15,15,15,15,15,15,
    15, 15,15,15,15,15,15,15,
    15, 15,15,15,15,15,15,15,
    7,15,15,15,3,15,15,11
};

/* Returns _TRUE if a legal move, _FALSE otherwise */
int make_move(U16 move) {
    void unmake_move();
    char flags = move_flags(move);
    char move_from, move_to;
    move_from = move_source(move);
    move_to = move_target(move);
    U64 from_to_bb = ((U64)1 << move_from) | ((U64)1 << move_to);
    /* Save irreversible information to stack */
    U16 data = encode_hist(((flags == 5) ? (p - 6*side_to_move) : piece_on_square[move_to]),castling_rights, en_passent_legal, en_passent_square);
    game_history[game_depth].move = move;
    game_history[game_depth].flags = data;
    game_history[game_depth].fifty_clock = fifty_move;
    game_history[game_depth].hash = hash;
    game_depth++;
    ply++;
    /* Update the bitboards and 8x8 board */
    int taken_piece = piece_on_square[move_to];
    bitboards[piece_on_square[move_from]] ^= from_to_bb;
    occupancies[BOTH] ^= from_to_bb;
    occupancies[side_to_move] ^= from_to_bb;
    piece_on_square[move_to] = piece_on_square[move_from];
    piece_on_square[move_from] = EMPTY;
    if (flags & 0x4) {
        /* Capture */
        if (flags == 5) {
            /* En Passent */
            piece_on_square[move_to + (side_to_move ? 8 : -8)] = EMPTY;
            bitboards[p - (6 * side_to_move)] ^= ((U64)1 << (move_to + (side_to_move ? 8 : -8)));
            occupancies[1 ^ side_to_move] ^= ((U64)1 << (move_to + (side_to_move ? 8 : -8)));
            occupancies[BOTH] ^= ((U64)1 << (move_to + (side_to_move ? 8 : -8)));
        } else {
            bitboards[taken_piece] ^= ((U64)1 << move_to);
            occupancies[1 ^ side_to_move] ^= ((U64)1 << move_to);
            occupancies[BOTH] ^= ((U64)1 << move_to);
        }
    }
    if (flags & 0x8) {
        /* Promotion */
        char promoted_piece = N + (6 * side_to_move) + (flags & 0x3);
        /* Fix piece on from square */
        piece_on_square[move_to] = promoted_piece;
        bitboards[promoted_piece] ^= ((U64)1 << move_to);
        bitboards[P + (6 * side_to_move)] ^= ((U64)1 << move_to);
    }
    if (flags == 2) {
        /* Kingside castles */
        char rook_home_square = (side_to_move ? h8 : h1);
        char rook_castles_square = (side_to_move ? f8 : f1);
        from_to_bb = ((U64)1 << rook_home_square) | ((U64)1 << rook_castles_square);
        piece_on_square[rook_castles_square] = R + (6 * side_to_move);
        piece_on_square[rook_home_square] = EMPTY;
        occupancies[side_to_move] ^= from_to_bb;
        occupancies[BOTH] ^= from_to_bb;
        bitboards[R + (6 * side_to_move)] ^= from_to_bb;
    } else if (flags == 3) {
        /* Queenside castles */
        char rook_home_square = (side_to_move ? a8 : a1);
        char rook_castles_square = (side_to_move ? d8 : d1);
        from_to_bb = ((U64)1 << rook_home_square) | ((U64)1 << rook_castles_square);
        piece_on_square[rook_castles_square] = R + (6 * side_to_move);
        piece_on_square[rook_home_square] = EMPTY;
        occupancies[side_to_move] ^= from_to_bb;
        occupancies[BOTH] ^= from_to_bb;
        bitboards[R + (6 * side_to_move)] ^= from_to_bb;
    }
    /* Update board state, including hash */
    en_passent_legal = (flags == 1) ? _TRUE : _FALSE;
    if (en_passent_legal)
        en_passent_square = (unsigned int) ((int)move_to + (side_to_move ? 8 : -8));
    side_to_move ^= 1;
    castling_rights &= castling_rights_update[move_from];
    castling_rights &= castling_rights_update[move_to];
    // King is never left in check, so don't need to unmake ever
    return _TRUE;
}

void unmake_move() {
    hist_t hist = game_history[--game_depth];
    U16 move = hist.move;
    char flags = move_flags(move);
    char move_from, move_to;
    move_from = move_source(move);
    move_to = move_target(move);
    U64 from_to_bb = ((U64)1 << move_from) | ((U64)1 << move_to);
    ply--;
    side_to_move ^= 1;
    /* Fix the bitboards and 8x8 board */
    bitboards[piece_on_square[move_to]] ^= from_to_bb;
    occupancies[BOTH] ^= from_to_bb;
    occupancies[side_to_move] ^= from_to_bb;
    piece_on_square[move_from] = piece_on_square[move_to];
    piece_on_square[move_to] = EMPTY;
    if (flags & 0x4) {
        /* Capture */
        if (flags == 5) {
            /* En Passent */
            piece_on_square[move_to + (side_to_move ? 8 : -8)] = (p - 6*side_to_move);
            bitboards[p - 6*side_to_move] ^= ((U64)1 << (move_to + (side_to_move ? 8 : -8)));
            occupancies[1 ^ side_to_move] ^= ((U64)1 << (move_to + (side_to_move ? 8 : -8)));
            occupancies[BOTH] ^= ((U64)1 << (move_to + (side_to_move ? 8 : -8)));
        } else {
            piece_on_square[move_to] = hist_capture(hist.flags);
            bitboards[hist_capture(hist.flags)] ^= ((U64)1 << move_to);
            occupancies[1 ^ side_to_move] ^= ((U64)1 << move_to);
            occupancies[BOTH] ^= ((U64)1 << move_to);
        }
    }
    if (flags & 0x8) {
        /* Promotion */
        char promoted_piece = 2 + (6 * side_to_move) + (flags & 0x3);
        /* Fix piece on from square */
        piece_on_square[move_from] = P + (6 * side_to_move);
        // The promoted piece was put back on the square the pawn started
        bitboards[promoted_piece] ^= ((U64)1 << move_from);
        bitboards[P + (6 * side_to_move)] ^= ((U64)1 << move_from);
    }
    if (flags == 2) {
        /* Kingside castles */
        char rook_home_square = (side_to_move ? h8 : h1);
        char rook_castles_square = (side_to_move ? f8 : f1);
        from_to_bb = ((U64)1 << rook_home_square) | ((U64)1 << rook_castles_square);
        piece_on_square[rook_castles_square] = EMPTY;
        piece_on_square[rook_home_square] = R + (6 * side_to_move);
        occupancies[side_to_move] ^= from_to_bb;
        occupancies[BOTH] ^= from_to_bb;
        bitboards[R + (6 * side_to_move)] ^= from_to_bb;
    } else if (flags == 3) {
        /* Queenside castles */
        char rook_home_square = (side_to_move ? a8 : a1);
        char rook_castles_square = (side_to_move ? d8 : d1);
        from_to_bb = ((U64)1 << rook_home_square) | ((U64)1 << rook_castles_square);
        piece_on_square[rook_castles_square] = EMPTY;
        piece_on_square[rook_home_square] = R + (6 * side_to_move);
        occupancies[side_to_move] ^= from_to_bb;
        occupancies[BOTH] ^= from_to_bb;
        bitboards[R + (6 * side_to_move)] ^= from_to_bb;
    }
    /* Return board state information */
    castling_rights = hist_castling(hist.flags);
    en_passent_legal = hist_ep_legal(hist.flags);
    en_passent_square = hist_ep_target(hist.flags);
    fifty_move = hist.fifty_clock;
    hash = hist.hash;
    return;
}

int make_capture(U16 move) {
    if (move_flags(move) & 0x4) {
        make_move(move);
        return _TRUE;
    }
    return _FALSE;
}

/* Perft */
U64 perft(int depth) {
    int n_moves, i,j;
    U64 nodes=0;
    if (depth == 0)
        return 1UL;
    generate_moves();
    for (i=moves_start_idx[ply]; i<moves_start_idx[ply+1];i++) {
        make_move(move_stack.moves[i].move);
        nodes += perft(depth - 1);
        unmake_move();
    }
    return nodes;
}


void perft_test(int depth) {
    int i;
    U64 nodes;
    int t;
    printf("Performance test\n");
    for (int i=1; i<=depth; i++) {
        t = get_time_ms();
        ply=0;
        nodes = perft(i);
        printf("\tDepth: %d \t Nodes: %lld \t Time: %dms\n",i, nodes, get_time_ms() - t);
    }
}

// Depth should be >= 2
void divide(int depth) {
    int n_moves, i,j;
    U64 nodes=0;
    generate_moves();
    for (i=moves_start_idx[ply]; i<moves_start_idx[ply+1];i++) {
        make_move(move_stack.moves[i].move);
        nodes = perft(depth - 1);
        print_move(move_stack.moves[i].move);
        printf(" %lld\n", nodes);
        unmake_move();
    }
}