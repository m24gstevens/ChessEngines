#include "moves.h"
#include "bitboard.h"

extern inline U64 bishopAttacks(enumSquare, U64);
extern inline U64 rookAttacks(enumSquare, U64);

void print_move(U16 move) {
    printf("%s%s",square_strings[move_from(move)],square_strings[move_to(move)]);
    if (is_promotion(move)) {
        printf("%c", promoted_pieces[move_promote_to(move)]);
    }
}

// Parse a move that hasn't been played on the board
U16 parse_move(char* s, board_t* board) {
    U16 move = 0;
    U8 square_from = (s[0]-'a') + 8*(s[1]-'1');
    U8 square_to = ((s[2]-'a') + 8*(s[3]-'1'));
    move |= square_from;
    move |= (U16)square_to << 6;
    if (board->squares[square_to] != _) {move |= 0x4000;}
    if (board->squares[square_from]%6==K) {
        if (square_to - square_from == 2) {move |= 0x2000;}
        else if (square_from - square_to == 2) {move |= 0x3000;};
    }

    if (board->squares[square_from]%6==P) {
        if (board->side==WHITE) {
            if (square_to > h7) {
                move |= 0x8000;
                switch (tolower(s[4])) {
                    case 'n':
                        break;
                    case 'b':
                        move |= 0x1000;
                        break;
                    case 'r':
                        move |= 0x2000;
                        break;
                    default:
                        move |= 0x3000;
                }
            }
            if (square_to - square_from == 16) {move |= 0x1000;}
            else if (square_to - square_from != 8 && board->squares[square_to] == _) {move |= 0x5000;}
        } else {
            if (square_to < a2) {
                move |= 0x8000;
                switch (tolower(s[4])) {
                    case 'n':
                        break;
                    case 'b':
                        move |= 0x1000;
                        break;
                    case 'r':
                        move |= 0x2000;
                        break;
                    default:
                        move |= 0x3000;
                }
            }
            if (square_from - square_to == 16) {move |= 0x1000;}
            else if (square_from - square_to != 8 && board->squares[square_to] == _) {move |= 0x5000;}
        }
    }
    return move;
}

static inline U64 n_attacks(U64 bb) {
    U64 temp = eastOne(bb) | westOne(bb);
    U64 attacks = northTwo(temp) | southTwo(temp);
    temp =  eastTwo(bb) | westTwo(bb);
    attacks |= northOne(temp) | southOne(temp);
    return attacks;
}

static inline U64 k_attacks(U64 bb) {
    U64 attacks = bb | eastOne(bb) | westOne(bb);
    attacks |= northOne(attacks) | southOne(attacks);
    return attacks ^ bb;
}

static inline U64 p_attacks(U64 bb, enumSide s) {
    U64 aside = westOne(bb) | eastOne(bb);
    return s ? southOne(aside) : northOne(aside); 
}

int is_square_attacked(board_t* board, enumSquare sq, enumSide side) {
    U64 bb = C64(1)<<sq;
    if (k_attacks(bb) & board->bitboards[K+6*side]) {return 1;}
    if (n_attacks(bb) & board->bitboards[N+6*side]) {return 1;}
    if (p_attacks(bb,1^side) & board->bitboards[P+6*side]) {return 1;}
    U64 attacks;
    if (bishopAttacks(sq,board->occupancies[BOTH]) & (board->bitboards[B+6*side] | board->bitboards[Q+6*side])) {return 1;}
    if (rookAttacks(sq,board->occupancies[BOTH]) & (board->bitboards[R+6*side] | board->bitboards[Q+6*side])) {return 1;}
    return 0;
}

int generate_moves(board_t* board, move_t* ms) {
    U64 pieces, open;
    move_t* p=ms;
    int from,to,side,flags,ct,king;

    side = board->side;
    ct=0;
    // Pawn moves

    const U64 seventhRank[2] = {0x00FF000000000000, 0xFF00};
    pieces = board->bitboards[P + 6*side] & ~seventhRank[side] & ~seventhRank[1^side];

    while (pieces) {
        from = bitscanForward(pieces);
        POP_LS1B(pieces);
        open = pawnAttackTable[side][from] & board->occupancies[1^side];
        while (open) {
            to = bitscanForward(open);
            POP_LS1B(open);
            (ms + (ct++))->move = encode_move(from,to,4);
        }
        to=from+8*(1-2*side);
        if (board->squares[to]==_) {
            (ms + (ct++))->move = encode_move(from,to,0);
        }
    }
    pieces = board->bitboards[P + 6*side] & ~seventhRank[side] & seventhRank[1^side];

    while (pieces) {
        from = bitscanForward(pieces);
        POP_LS1B(pieces);
        open = pawnAttackTable[side][from] & board->occupancies[1^side];
        while (open) {
            to = bitscanForward(open);
            POP_LS1B(open);
            (ms + (ct++))->move = encode_move(from,to,4);
        }
        to=from+8*(1-2*side);
        if (board->squares[to]==_) {
            (ms + (ct++))->move = encode_move(from,to,0);
            to=from+16*(1-2*side);
            if (board->squares[to]==_) {
                (ms + (ct++))->move = encode_move(from,to,1);
            }
        }
    }

    pieces = board->bitboards[P + 6*side] & seventhRank[side];

    while (pieces) {
        from = bitscanForward(pieces);
        POP_LS1B(pieces);
        open = pawnAttackTable[side][from] & board->occupancies[1^side];
        while (open) {
            to = bitscanForward(open);
            POP_LS1B(open);
            for (int p=0;p<4;p++) {(ms + (ct++))->move = encode_move(from,to,12+p);}
        }

        to=from+8*(1-2*side);
        if (board->squares[to]==_) {
            for (int p=0;p<4;p++) {(ms + (ct++))->move = encode_move(from,to,8+p);}
        }
    }

    // King moves

    pieces = board->bitboards[K + 6*side];
    king = bitscanForward(pieces);
    open = kingAttackTable[king] & ~board->occupancies[side];
    while (open) {
        to = bitscanForward(open);
        POP_LS1B(open);
        flags = (board->squares[to]==_) ? 0 : 4;
        (ms + (ct++))->move = encode_move(king,to,flags);
    }


    // Special moves
    // EP
    if (board->ep_square != -1) {
        pieces = pawnAttackTable[1^side][board->ep_square] & board->bitboards[P+6*side];
        while (pieces) {
            from = bitscanForward(pieces);
            POP_LS1B(pieces);
            (ms + (ct++))->move = encode_move(from,board->ep_square,5);
        }
    }
    // O-O
    if (board->castle_flags & (1<<(2*side)) && board->squares[king+1]==_ && board->squares[king+2]) {
        if (!is_square_attacked(board,king+1,1^side)) {(ms + (ct++))->move = encode_move(king,king+2,2);}
    }

    // O-O-O
    if (board->castle_flags & (1<<(2*side + 1)) && board->squares[king-1]==_ && board->squares[king-2] && board->squares[king-3]) {
        if (!is_square_attacked(board,king-1,1^side)) {(ms + (ct++))->move = encode_move(king,king-2,3);}
    }

    // Knight moves

    pieces = board->bitboards[N + 6*side];
    while (pieces) {
        from = bitscanForward(pieces);
        POP_LS1B(pieces);
        open = knightAttackTable[from] & ~board->occupancies[side];
        while (open) {
            to = bitscanForward(open);
            POP_LS1B(open);
            flags = (board->squares[to]==_) ? 0 : 4;
            (ms + (ct++))->move = encode_move(from,to,flags);
        }
    }
    

    // Bishop moves
    pieces = board->bitboards[B + 6*side] | board->bitboards[Q + 6*side];
    while (pieces) {
        from = bitscanForward(pieces);
        POP_LS1B(pieces);
        open = bishopAttacks(from,board->occupancies[BOTH]) & ~board->occupancies[side];
        while (open) {
            to = bitscanForward(open);
            POP_LS1B(open);
            flags = (board->squares[to]==_) ? 0 : 4;
            (ms + (ct++))->move = encode_move(from,to,flags); 
        }
    }

    // Rook moves
    pieces = board->bitboards[R + 6*side] | board->bitboards[Q + 6*side];
    while (pieces) {
        from = bitscanForward(pieces);
        POP_LS1B(pieces);
        open = rookAttacks(from,board->occupancies[BOTH]) & ~board->occupancies[side];
        while (open) {
            to = bitscanForward(open);
            POP_LS1B(open);
            flags = (board->squares[to]==_) ? 0 : 4;
            (ms + (ct++))->move = encode_move(from,to,flags); 
        }
    }


    return ct;
}


