#include "board.h"
#include "common.h"

char *piece_characters = "KPNBRQkpnbrq.";
enumPiece char_to_piece_code[] = {
    ['K'] = K,
    ['P'] = P,
    ['N'] = N,
    ['B'] = B,
    ['R'] = R,
    ['Q'] = Q,
    ['k'] = k,
    ['p'] = p,
    ['n'] = n,
    ['b'] = b,
    ['q'] = q,
    ['r'] = r,
    ['.'] = _
};

char *square_strings[64] = {
    "a1","b1","c1","d1","e1","f1","g1","h1",
    "a2","b2","c2","d2","e2","f2","g2","h2",
    "a3","b3","c3","d3","e3","f3","g3","h3",
    "a4","b4","c4","d4","e4","f4","g4","h4",
    "a5","b5","c5","d5","e5","f5","g5","h5",
    "a6","b6","c6","d6","e6","f6","g6","h6",
    "a7","b7","c7","d7","e7","f7","g7","h7",
    "a8","b8","c8","d8","e8","f8","g8","h8"
};

char *promoted_pieces= "nbrq";

void print_board(board_t board) {
    printf("\n");
    for (int i=7; i>=0; i--) {
        printf("%d   ", i+1);
        for (int j=0; j<8;j++) {
            printf("%c ", piece_characters[board.squares[8*i + j]]);
        }
        printf("\n");
    }
    printf("\n    ");
    for (int j=0; j<8; j++)
        printf("%c ", 'a' + j);
    printf("\n\nBoard State:\n");
    printf("Side to move: %d\t", board.side);
    printf("Castling rights: %d\n", board.castle_flags);
    if (board.ep_square != -1) {
        printf("En Passent Target: %c%c\n", 'a'+board.ep_square%8, '1'+board.ep_square/8);
    } else 
        printf("\n");
}

void parse_fen(char* fen, board_t* board) {
    memset(&board->bitboards,0,sizeof(board->bitboards));
    memset(&board->occupancies,0,sizeof(board->occupancies));
    memset(&board->squares,_,sizeof(board->squares));

    board->ep_square = 0;
    board->castle_flags = 0;
    board->side = WHITE;

    for (int r=7; r >= 0; r--) {
        for (int f=0; f <= 8; f++) {
            if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z')) {
                char piece = char_to_piece_code[*fen++];
                board->bitboards[piece] |= ((U64)1 << (8*r + f));
                board->squares[8*r + f] = piece;
                board->occupancies[piece/6] |= ((U64)1 << (8*r + f));
            }
            if (*fen >= '0' && *fen <= '9') {
                int offset = *fen++ - '0';
                if (board->squares[8*r + f] == _) f--;
                f += offset;
            }
            if (*fen == '/') {
                fen++;
                break;
            }
        }
    }
    /* Side to move */
    if (*(++fen) == 'b') {board->side = BLACK;};
    fen += 2;
    /* Castling rights */
    while (*fen != ' ') {
        switch(*fen) {
            case 'K': board->castle_flags |= WCK; break;
            case 'Q': board->castle_flags |= WCQ; break;
            case 'k': board->castle_flags |= BCK; break;
            case 'q': board->castle_flags |= BCQ; break;
        }
        fen++;
    }
    fen++;
    /* en passent square */
    if (*fen != '-') {
        int fl = *fen++ - 'a';
        int rk = *fen - '1';
        board->ep_square = 8 * rk + fl;
    } else {board->ep_square = -1;}

    /* Fifty move clock */
    board->rule50 = atoi(fen + 2);
    /* occupancies */
    board->occupancies[BOTH] = board->occupancies[WHITE] | board->occupancies[BLACK];
}