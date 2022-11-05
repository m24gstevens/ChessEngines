#include "moves.h"
#include "bitboard.h"

extern inline U64 bishopAttacks(enumSquare, U64);
extern inline U64 rookAttacks(enumSquare, U64);

void print_move(U16 move) {
    printf("%s%s",square_strings[MOVE_FROM(move)],square_strings[MOVE_TO(move)]);
    if (IS_PROMOTION(move)) {
        printf("%c", promoted_pieces[MOVE_PROMOTE_TO(move)]);
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
    if (board->castle_flags & (1<<(2*side)) && board->squares[king+1]==_ && board->squares[king+2]==_) {
        if (!is_square_attacked(board,king,1^side) && !is_square_attacked(board,king+1,1^side)) {(ms + (ct++))->move = encode_move(king,king+2,2);}
    }

    // O-O-O
    if (board->castle_flags & (1<<(2*side + 1)) && board->squares[king-1]==_ && board->squares[king-2]==_ && board->squares[king-3]==_) {
        if (!is_square_attacked(board,king,1^side) && !is_square_attacked(board,king-1,1^side)) {(ms + (ct++))->move = encode_move(king,king-2,3);}
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

const U8 castling_rights_update[64] = {
    13,15,15,15,12,15,15,14,
    15, 15,15,15,15,15,15,15,
    15, 15,15,15,15,15,15,15,
    15, 15,15,15,15,15,15,15,
    15, 15,15,15,15,15,15,15,
    15, 15,15,15,15,15,15,15,
    15, 15,15,15,15,15,15,15,
    7,15,15,15,3,15,15,11
};


int make_move(board_t* board, move_t* move, hist_t* undo) {
    U8 from, to, flags, promoted;
    U16 mov;
    enumSquare moved, captured, rook_home, rook_castles, king;
    enumSide side;
    U64 from_to_bb, from_bb, to_bb, ep_bb;

    void unmake_move(board_t*, move_t*, hist_t*);

    mov = move->move;
    from = MOVE_FROM(mov);
    to = MOVE_TO(mov);
    flags = MOVE_FLAGS(mov);
    moved = board->squares[from];
    captured = board->squares[to];
    side = board->side;

    from_bb = (C64(1) << from);
    to_bb = (C64(1) << to);
    from_to_bb = (C64(1) << from) | (C64(1) << to);

    // Save irreversibles
    undo->captured = captured;
    undo->ep_square = board->ep_square;
    undo->castle_flags = board->castle_flags;
    undo->rule50 = board->rule50;
    // Update occupancies
    board->bitboards[moved] ^= from_to_bb;
    board->occupancies[side] ^= from_to_bb;
    board->occupancies[BOTH] ^= from_to_bb;
    board->squares[from] = _;
    board->squares[to] = moved;

    // Captures, incl. EP
    if (flags & 0x4) {
        if (flags == 5) {
            ep_bb = (C64(1) << (to + 16*side - 8));
            board->squares[to + 16*side - 8] = _;
            board->bitboards[p - 6*side] ^= ep_bb;
            board->occupancies[BOTH] ^= ep_bb;
            board->occupancies[1^side] ^= ep_bb;
        } else {
            board->bitboards[captured] ^= to_bb;
            board->occupancies[BOTH] ^= to_bb;
            board->occupancies[1^side] ^= to_bb;
        }
    }
    // Promotion
    if (flags & 0x8) {
        promoted = N + 6*side + MOVE_PROMOTE_TO(mov);
        board->squares[to] = promoted;
        board->bitboards[promoted] ^= to_bb;
        board->bitboards[P + 6*side] ^= to_bb;
    }
    // Castling
    if (flags == 2) {
        rook_home = h1 + 56*side;
        rook_castles = f1 + 56*side;
        from_to_bb = (C64(1) << rook_home) | (C64(1) << rook_castles);
        board->squares[rook_castles] = R + 6*side;
        board->squares[rook_home] = _;
        board->occupancies[side] ^= from_to_bb;
        board->occupancies[BOTH] ^= from_to_bb;
        board->bitboards[R + 6*side] ^= from_to_bb;
    } else if (flags == 3) {
        rook_home = a1 + 56*side;
        rook_castles = d1 + 56*side;
        from_to_bb = (C64(1) << rook_home) | (C64(1) << rook_castles);
        board->squares[rook_castles] = R + 6*side;
        board->squares[rook_home] = _;
        board->occupancies[side] ^= from_to_bb;
        board->occupancies[BOTH] ^= from_to_bb;
        board->bitboards[R + 6*side] ^= from_to_bb;
    }
    // Update other board state
    board->side ^= 1;
    board->castle_flags &= castling_rights_update[from];
    board->castle_flags &= castling_rights_update[to];
    if ((flags & 0x4) || (moved == P) || (moved == p)) {board->rule50 = 0;}
    else {board->rule50++;}
    if (flags==1) {
        board->ep_square = to + 16*side - 8;
    } else {
        board->ep_square = -1;
    }

    king = bitscanForward(board->bitboards[K+6*side]);
    if (is_square_attacked(board,king,1^side)) {
        unmake_move(board,move,undo);
        return 0;
    }
    return 1;
}

void unmake_move(board_t* board, move_t* move, hist_t* undo) {
    U8 from, to, flags, promoted;
    U16 mov;
    enumSquare moved, captured, rook_home, rook_castles, king;
    enumSide side;
    U64 from_to_bb, from_bb, to_bb, ep_bb;

    mov = move->move;
    from = MOVE_FROM(mov);
    to = MOVE_TO(mov);
    flags = MOVE_FLAGS(mov);
    moved = board->squares[to];
    captured = undo->captured;
    side = 1^board->side;

    from_bb = (C64(1) << from);
    to_bb = (C64(1) << to);
    from_to_bb = (C64(1) << from) | (C64(1) << to);

    // Update occupancies
    board->bitboards[moved] ^= from_to_bb;
    board->occupancies[side] ^= from_to_bb;
    board->occupancies[BOTH] ^= from_to_bb;
    board->squares[to] = _;
    board->squares[from] = moved;

    // Captures incl. EP
    if (flags & 0x4) {
        if (flags == 5) {
            ep_bb = (C64(1) << (to + 16*side - 8));
            board->squares[to + 16*side - 8] = p - 6*side;
            board->bitboards[p - 6*side] ^= ep_bb;
            board->occupancies[BOTH] ^= ep_bb;
            board->occupancies[1^side] ^= ep_bb;
        } else {
            board->squares[to] = captured;
            board->bitboards[captured] ^= to_bb;
            board->occupancies[1^side] ^= to_bb;
            board->occupancies[BOTH] ^= to_bb;
        }
    }
    // Promotion
    if (flags & 0x8) {
        promoted = N + 6*side + MOVE_PROMOTE_TO(mov);
        board->squares[from] = P + 6*side;
        board->bitboards[promoted] ^= from_bb;
        board->bitboards[P + 6*side] ^= from_bb;
    }
    // Castling
    if (flags == 2) {
        rook_home = h1 + 56*side;
        rook_castles = f1 + 56*side;
        from_to_bb = (C64(1) << rook_home) | (C64(1) << rook_castles);
        board->squares[rook_castles] = _;
        board->squares[rook_home] = R + 6*side;
        board->occupancies[side] ^= from_to_bb;
        board->occupancies[BOTH] ^= from_to_bb;
        board->bitboards[R + 6*side] ^= from_to_bb;
    } else if (flags == 3) {
        rook_home = a1 + 56*side;
        rook_castles = d1 + 56*side;
        from_to_bb = (C64(1) << rook_home) | (C64(1) << rook_castles);
        board->squares[rook_castles] = _;
        board->squares[rook_home] = R + 6*side;
        board->occupancies[side] ^= from_to_bb;
        board->occupancies[BOTH] ^= from_to_bb;
        board->bitboards[R + 6*side] ^= from_to_bb;
    }
    // Restore irreversibles
    board->castle_flags = undo->castle_flags;
    board->ep_square = undo->ep_square;
    board->rule50 = undo->rule50;
    board->side ^= 1;
}

long perft(board_t* board, move_t* ms, hist_t* undo, int depth) {
    int n_moves, i, res;
    long nodes = 0;
    if (depth == 0) {return 1L;}
    n_moves = generate_moves(board,ms);
    for (i=0;i<n_moves;i++) {
        if (make_move(board,ms+i,undo)) {
            nodes += perft(board,ms+n_moves,undo+1,depth-1);
            unmake_move(board,ms+i,undo);
        }
    }
    return nodes;
}

void divide(board_t* board, move_t* ms, hist_t* undo, int depth) {
    int n_moves, i, res;
    long nodes;
    n_moves = generate_moves(board,ms);
    for (i=0;i<n_moves;i++) {
        if (make_move(board,ms+i,undo)) {
            print_move((ms+i)->move);
            printf(" %d\n",perft(board,ms+n_moves,undo+1,depth-1));
            unmake_move(board,ms+i,undo);
        }
    }
}