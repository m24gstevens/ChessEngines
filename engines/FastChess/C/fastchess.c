#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>
typedef uint64_t U64;
typedef uint16_t MOV;
#define BITSCAN(bb) __builtin_ffsll(bb)
#define POPCOUNT(bb) __builtin_popcountll(bb)
#define OFF -1
#define S_FILE(sq) ((sq) & 7)
#define S_RANK(sq) ((sq) >> 3)
#define ENCODE_MOVE(flg,src,trg) ((flg) << 12) | ((src) << 6) | (trg)
#define MOVE_SRC(mov) (((mov) >> 6) & 0x3F)
#define MOVE_TRG(mov) ((mov) & 0x3F)
#define MOVE_FLG(mov) ((mov) >> 12)

/* Board representation
Data structures used:
-- 2 array[6] of bitboards to represent the occupation sets for each piece
-- Global variables for each of the position "flags"
-- array[64] as an occupation set. We represent the occupying pieces by an encoding:
Store integers in the array. By bit,
-- 1st bit is 0 if the square is unoccupied. Otherwise, this is 1.
-- 2nd bit is 0 if the piece is white. Otherwise, this is 1.
-- Bits 3-5 encode the piece:
0: King     1: Pawn     2: Knight      3: Bishop        4: Rook     5: Queen */

U64 w_pieces[6] = {0x0000000000000010, 0x000000000000FF00, 0x0000000000000042, 0x0000000000000024, 0x0000000000000081, 0x0000000000000008};
U64 b_pieces[6] = {0x1000000000000000, 0x00FF000000000000, 0x4200000000000000, 0x2400000000000000, 0x8100000000000000, 0x0800000000000000};
U64 *all_pieces[2] = {w_pieces, b_pieces};

char colour = 0;
char castling_rights = 0xF;
char ep_legal = 0;
char ep_square = 0;
char halfmove_clock = 0;
char move_counter = 1;

int occupied[64] = {
    17,9,13,21,1,13,9,17,
    5,5,5,5,5,5,5,5,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    7,7,7,7,7,7,7,7,
    19,11,15,23,3,15,11,19
};

int mailbox[120] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,0,1,2,3,4,5,6,7,-1,-1,8,9,10,11,12,13,14,15,-1,
    -1,16,17,18,19,20,21,22,23,-1,-1,24,25,26,27,28,29,30,31,-1,
    -1,32,33,34,35,36,37,38,39,-1,-1,40,41,42,43,44,45,46,47,-1,
    -1,48,49,50,51,52,53,54,55,-1,-1,56,57,58,59,60,61,62,63,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

int to_mailbox[64] = {
21,22,23,24,25,26,27,28,
31,32,33,34,35,36,37,38,
41,42,43,44,45,46,47,48,
51,52,53,54,55,56,57,58,
61,62,63,64,65,66,67,68,
71,72,73,74,75,76,77,78,
81,82,83,84,85,86,87,88,
91,92,93,94,95,96,97,98
};

/* Debugging and helper routines */

void print_bb(U64 bb) {
    int i,j;
    for (i=7;i>=0;i--) {
        printf("%d\t",i+1);
        for (j=0;j<8;j++)
            printf(" %d",((bb >> (8*i + j)) & 1));
        printf("\n");
    }
    printf(" \t A B C D E F G H\n");
}

char *piece_strings[12] = {"WK","bk","WP","bp","WN","bn",
"WB","bb","WR","br","WQ","bq"};

void print_board() {
    int i,j,occupying;
    for (i=7;i>=0;i--) {
        printf("%d\t",i+1);
        for (j=0;j<8;j++) {
            occupying = occupied[8*i + j];
            if (!occupying)
                printf(" .. ");
            else
                printf(" %s ",piece_strings[occupying >> 1]);
        }
        printf("\n");
    }
    printf(" \t A  B  C  D  E  F  G  H\n");
}

void reinitialize() {
    int i,j,n,rank,file;
    char c;

    //Update piece sets
    for (i=0; i<6; i++) {
        w_pieces[i] = 0;
        b_pieces[i] = 0;
    }


    for (i=7; i>=0; i--) {
        for (j=0; j < 8; j++) 
            occupied[8*i + j] = 0;
        }
    for (i=7; i>=0; i--) {
        for (j=0; (c = getchar()) != '/' && c != ' ';)
            switch (c) {
                case 'p':
                    b_pieces[1] |= (U64)1 << ((i << 3) + j); 
                    occupied[8*i + j++] = 7;
                    break;
                case 'k':
                    b_pieces[0] = (U64)1 << ((i << 3) + j);
                    occupied[8*i + j++] = 3;
                    break;
                case 'n':
                    b_pieces[2] |= (U64)1 << ((i << 3) + j);
                    occupied[8*i + j++] = 11;
                    break;
                case 'b':
                    b_pieces[3] |= (U64)1 << ((i << 3) + j);
                    occupied[8*i + j++] = 15;
                    break;
                case 'r':
                    b_pieces[4] |= (U64)1 << ((i << 3) + j);
                    occupied[8*i + j++] = 19;
                    break;
                case 'q':
                    b_pieces[5] |= (U64)1 << ((i << 3) + j);
                    occupied[8*i + j++] = 23;
                    break;
                case 'P':
                    w_pieces[1] |= (U64)1 << ((i << 3) + j);
                    occupied[8*i + j++] = 5;
                    break;
                case 'K':
                    w_pieces[0] = (U64)1 << ((i << 3) + j);
                    occupied[8*i + j++] = 1;
                    break;
                case 'N':
                    w_pieces[2] |= (U64)1 << ((i << 3) + j);
                    occupied[8*i + j++] = 9;
                    break;
                case 'B':
                    w_pieces[3] |= (U64)1 << ((i << 3) + j);
                    occupied[8*i + j++] = 13;
                    break;
                case 'R':
                    w_pieces[4] |= (U64)1 << ((i << 3) + j);
                    occupied[8*i + j++] = 17;
                    break;
                case 'Q':
                    w_pieces[5] |= (U64)1 << ((i << 3) + j);
                    occupied[8*i + j++] = 21;
                    break;
                default:
                    j += c - '0';
                    break;
            }
    }
    colour = 0;
    castling_rights = 0;
    ep_legal = 0;
    ep_square = 0;
    halfmove_clock = 0;
    move_counter = 0;
    if ((c = getchar()) == 'b')
        colour= 1;
    getchar();  //Space

    while (!isspace(c = getchar()))     // Castling Rights
        switch (c) {
            case 'K':
                castling_rights |= 1;
                break;
            case 'Q':
                castling_rights |= 2;
                break;
            case 'k':
                castling_rights |= 4;
                break;
            case 'q':
                castling_rights |= 8;
                break;
        }
    
    if (isalpha(c = getchar())) {
        file = c - 'a';
        rank = (c = getchar()) - '1';
        ep_legal = 1;
        ep_square = 8*rank + file;
    }
    scanf("%d", &n);
    halfmove_clock = n;
    scanf("%d", &n);
    move_counter = n;

}

void print_square(int sq) {
    printf("%c%c",'a' + (sq & 7), '1' + (sq >> 3));
}

void summarise_position() {
    print_board();
    printf("Colour: %c\n", (colour ? 'b' : 'w'));
    printf("White Castling: %c%c\n", ((castling_rights & 1) ? 'K' : '-'),((castling_rights & 2) ? 'Q' : '-'));
    printf("Black Castling: %c%c\n", ((castling_rights & 4) ? 'K' : '-'),((castling_rights & 8) ? 'Q' : '-'));
    if (ep_legal) {
        printf("En Passent Target: ");
        print_square(ep_square);
        printf("\n");}
    printf("Halfmove clock: %d\n",halfmove_clock);
    printf("Move counter: %d\n",move_counter);
}

/* index_move_to Prints out the square name for the given square index in 'A1','A2',... enumeration */
void index_move_to(char square) {
    printf("%c%c\n", S_FILE(square) + 'a', S_RANK(square) + '1');
}

/* Move generation
As engine is fixed depth (Max 6 for now), we store moves in arrays of size 70 (More than number of legal moves in practically all positions)
Each array is designed to hold moves for a given search depth
We signal that we're out of legal moves by a 0 in the array.
Moves are encoded in the following format:
-- Lowest 6 bits encode the "to" square in 'A1','A2',... enumeration
-- Next 6 bits encode the "from" square
-- Highest 4 bits encode the flags. The flag setup is exactly the same as for the Python version
So can store all the move information in a uint16_t */

/* move_decode: Prints out the move using from and to squares */
void move_decode(MOV move) {
    printf("%c%c to %c%c\n", S_FILE(MOVE_SRC(move)) + 'a', S_RANK(MOVE_SRC(move)) + '1',
     S_FILE(MOVE_TRG(move)) + 'a', S_RANK(MOVE_TRG(move)) + '1');
}

MOV allmoves[6][70];

int king_mb_offsets[8] = {-11,-10,-9,-1,1,9,10,11};
int king_true_offsets[8] = {-9,-8,-7,-1,1,7,8,9};
int knight_mb_offsets[8] = {-21,-19,-12,-8,8,12,19,21};
int knight_true_offsets[8] = {-17,-15,-10,-6,6,10,15,17};
int bishop_mb_offsets[4] = {-11,-9,9,11};
int bishop_true_offsets[4] = {-9,-7,7,9};
int rook_mb_offsets[4] = {-10,-1,1,10};
int rook_true_offsets[4] = {-8,-1,1,8};

char generate_castling_available();

/* main move generation routine */
int generate_moves(int index) {
    int count = 0;
    int i;
    int king_captured=0;
    int double_reverse_colour;
    int king_start, enemy_king;
    int mb_direction,true_direction, target_square, start_square, target_piece, piece_square, test, t, t2;
    char castling_sides;
    U64 pawns, knights, bishops, rooks;
    enemy_king = BITSCAN(*(all_pieces[1^colour])) - 1;
    double_reverse_colour = (1^colour) << 1;

    //King
    piece_square = BITSCAN(*(all_pieces[colour])) - 1;
    for (i=0;i<8;i++) {
        target_square = to_mailbox[piece_square];
        mb_direction = king_mb_offsets[i];
        test = mailbox[target_square + mb_direction];
        if (test != OFF) {
            true_direction = king_true_offsets[i];
            target_piece = occupied[piece_square + true_direction];
            if (piece_square + true_direction == enemy_king)
                king_captured = 1;
            if (!target_piece)
                allmoves[index][count++] = ENCODE_MOVE(0,piece_square,piece_square + true_direction);
            else if ((target_piece & 2) == double_reverse_colour)
                allmoves[index][count++] = ENCODE_MOVE(4,piece_square,piece_square + true_direction);
        }
    }

    //Pawns
    pawns = *(all_pieces[colour] + 1);
    while (pawns) {
        piece_square = BITSCAN(pawns) - 1;
        pawns ^= (U64)1 << piece_square;
        if (colour) {
            if (piece_square >= 16) {
                if (S_FILE(piece_square)) {
                    t = occupied[piece_square - 9];
                    if (t && ((t & 2) == double_reverse_colour)) {
                        allmoves[index][count++] = ENCODE_MOVE(4,piece_square,piece_square - 9);
                        if (piece_square - 9==enemy_king)
                            king_captured = 1; }
                    }
                if (S_FILE(piece_square) != 7) {
                    t = occupied[piece_square - 7];
                    if (t && ((t & 2) == double_reverse_colour)) {
                        allmoves[index][count++] = ENCODE_MOVE(4,piece_square,piece_square - 7);
                        if (piece_square - 7==enemy_king)
                            king_captured = 1; }
                    }
                t = occupied[piece_square - 8];
                if (!t) {
                    allmoves[index][count++] = ENCODE_MOVE(0,piece_square,piece_square - 8);
                    if (piece_square >= 48) {
                        t2 = occupied[piece_square - 16];
                        if (!t2)
                            allmoves[index][count++] = ENCODE_MOVE(1,piece_square,piece_square - 16);
                    }
                }
            } else {
                if (S_FILE(piece_square)) {
                    t = occupied[piece_square - 9];
                    if (t && ((t & 2) == double_reverse_colour)) {
                        for (i=0; i<4; i++) {
                            allmoves[index][count++] = ENCODE_MOVE(12+i,piece_square,piece_square - 9);
                            if (piece_square - 9==enemy_king)
                                king_captured = 1; }
                    }
                }
                if (S_FILE(piece_square) != 7) {
                    t = occupied[piece_square - 7];
                    if (t && ((t & 2) == double_reverse_colour)) {
                        for (i=0; i<4; i++) {
                            allmoves[index][count++] = ENCODE_MOVE(12+i,piece_square,piece_square - 7);
                            if (piece_square - 7==enemy_king)
                                king_captured = 1; }
                    }
                }
                t = occupied[piece_square - 8];
                if (!t) {
                    for (i=0; i<4; i++)
                        allmoves[index][count++] = ENCODE_MOVE(8+i,piece_square,piece_square - 8);
                }   
            }
        } else {
            if (piece_square < 48) {
                if (S_FILE(piece_square)) {
                    t = occupied[piece_square + 7];
                    if (t && ((t & 2) == double_reverse_colour)) {
                        allmoves[index][count++] = ENCODE_MOVE(4,piece_square,piece_square + 7);
                        if (piece_square + 7==enemy_king)
                            king_captured = 1; }
                    }
                if (S_FILE(piece_square) != 7) {
                    t = occupied[piece_square + 9];
                    if (t && ((t & 2) == double_reverse_colour)) {
                        allmoves[index][count++] = ENCODE_MOVE(4,piece_square,piece_square + 9);
                        if (piece_square + 7==enemy_king)
                            king_captured = 1; }
                    }
                t = occupied[piece_square + 8];
                if (!t) {
                    allmoves[index][count++] = ENCODE_MOVE(0,piece_square,piece_square + 8);
                    if (piece_square < 16) {
                        t2 = occupied[piece_square + 16];
                        if (!t2)
                            allmoves[index][count++] = ENCODE_MOVE(1,piece_square,piece_square + 16);
                    }
                }
            } else {
                if (S_FILE(piece_square)) {
                    t = occupied[piece_square + 7];
                    if (t && ((t & 2) == double_reverse_colour)) {
                        for (i=0; i<4; i++) {
                            allmoves[index][count++] = ENCODE_MOVE(12+i,piece_square,piece_square + 7);
                            if (piece_square + 7==enemy_king)
                                king_captured = 1; }
                    }
                }
                if (S_FILE(piece_square) != 7) {
                    t = occupied[piece_square + 9];
                    if (t && ((t & 2) == double_reverse_colour)) {
                        for (i=0; i<4; i++) {
                            allmoves[index][count++] = ENCODE_MOVE(12+i,piece_square,piece_square + 9);
                            if (piece_square + 7==enemy_king)
                                king_captured = 1; }
                    }
                }
                t = occupied[piece_square + 8];
                if (!t) {
                    for (i=0; i<4; i++)
                        allmoves[index][count++] = ENCODE_MOVE(8+i,piece_square,piece_square + 8);
                }   
            }
        }
    }

    //En Passent
    if (ep_legal) {
        if (colour) {
            if (S_FILE(ep_square)) {
                t = occupied[ep_square + 7];
                if (t==7)
                    allmoves[index][count++] = ENCODE_MOVE(5,ep_square + 7,ep_square);
            }
            if (S_FILE(ep_square) != 7) {
                t = occupied[ep_square + 9];
                if (t==7)
                    allmoves[index][count++] = ENCODE_MOVE(5,ep_square+9,ep_square);
            }
        } else {
            if (S_FILE(ep_square)) {
                t = occupied[ep_square - 9];
                if (t==5)
                    allmoves[index][count++] = ENCODE_MOVE(5,ep_square - 9,ep_square);
            }
            if (S_FILE(ep_square) != 7) {
                t = occupied[ep_square - 7];
                if (t==5)
                    allmoves[index][count++] = ENCODE_MOVE(5,ep_square - 7,ep_square);
            }
        }
    }

    //Castles
    castling_sides = (castling_rights >> (colour ? 2 : 0)) & 3;
    castling_sides &= generate_castling_available();
    king_start = colour ? 60 : 4;
    if (castling_sides & 1) {
        if ((!occupied[king_start + 1]) && (!occupied[king_start + 2]) && (occupied[king_start + 3] == (colour ? 19 : 17)))
            allmoves[index][count++] = ENCODE_MOVE(2,king_start,king_start + 2);
    }
    if (castling_sides & 2) {
        if ((!occupied[king_start - 1]) && (!occupied[king_start - 2]) && (!occupied[king_start - 3]) && (occupied[king_start - 4] == (colour ? 19 : 17)))
            allmoves[index][count++] = ENCODE_MOVE(3,king_start,king_start - 2);
    }

    //Knights
    knights = *(all_pieces[colour] + 2);
    while (knights) {
        piece_square = BITSCAN(knights) - 1;
        knights ^= (U64)1 << piece_square;
        for (i=0;i<8;i++) {
            target_square = to_mailbox[piece_square];
            mb_direction = knight_mb_offsets[i];
            test = mailbox[target_square + mb_direction];
            if (test != OFF) {
                true_direction = knight_true_offsets[i];
                target_piece = occupied[piece_square + true_direction];
                if (piece_square + true_direction == enemy_king)
                    king_captured = 1;
                if (!target_piece)
                    allmoves[index][count++] = ENCODE_MOVE(0,piece_square,piece_square + true_direction);
                else if ((target_piece & 2) == double_reverse_colour)
                    allmoves[index][count++] = ENCODE_MOVE(4,piece_square,piece_square + true_direction);
            }
        }
    }

    //bishops
    bishops = *(all_pieces[colour] + 3) | *(all_pieces[colour] + 5);
    while (bishops) {
        piece_square = BITSCAN(bishops) - 1;
        bishops ^= (U64)1 << piece_square;
        start_square = piece_square;
        for (i=0;i<4;i++) {
            piece_square = start_square;
            target_square = to_mailbox[start_square];
            mb_direction = bishop_mb_offsets[i];
            true_direction = bishop_true_offsets[i];
            while (1) {
                target_square += mb_direction;
                test = mailbox[target_square];
                if (test != OFF) {
                    piece_square += true_direction;
                    target_piece = occupied[piece_square];
                    if (!target_piece)
                        allmoves[index][count++] = ENCODE_MOVE(0,start_square,piece_square);
                    else {
                        if ((target_piece & 2) == double_reverse_colour) {
                            allmoves[index][count++] = ENCODE_MOVE(4,start_square,piece_square);
                            if (piece_square == enemy_king)
                                king_captured = 1;
                        }
                        break;
                    }
                } else
                    break;
            }
        }
    }

    //rooks
    rooks = *(all_pieces[colour] + 4) | *(all_pieces[colour] + 5);
    while (rooks) {
        piece_square = BITSCAN(rooks) - 1;
        rooks ^= (U64)1 << piece_square;
        start_square = piece_square;
        for (i=0;i<4;i++) {
            piece_square = start_square;
            target_square = to_mailbox[start_square];
            mb_direction = rook_mb_offsets[i];
            true_direction = rook_true_offsets[i];
            while (1) {
                target_square += mb_direction;
                test = mailbox[target_square];
                if (test != OFF) {
                    piece_square += true_direction;
                    target_piece = occupied[piece_square];
                    if (!target_piece)
                        allmoves[index][count++] = ENCODE_MOVE(0,start_square,piece_square);
                    else {
                        if ((target_piece & 2) == double_reverse_colour) {
                            allmoves[index][count++] = ENCODE_MOVE(4,start_square,piece_square);
                            if (piece_square == enemy_king)
                                king_captured = 1;
                        }
                        break;
                    }
                } else
                    break;
            }
        }
    }
    allmoves[index][count] = 0;
    return king_captured;
}

// Check castling legality
char generate_castling_available() {
    int other_colour = 1^colour;
    char available = 3;
    int kingside_mb_square = other_colour ? 26 : 96;
    int queenside_mb_square = other_colour ? 24 : 94;
    int king_square = other_colour ? 25 : 95;
    int i;
    int mb_direction,true_direction, target_square, start_square, target_piece, piece_square, test;
    U64 knights,bishops,rooks, pawns;

    //king
    piece_square = BITSCAN(*(all_pieces[1^colour])) - 1;
    for (i=0;i<8;i++) {
        target_square = to_mailbox[piece_square];
        mb_direction = king_mb_offsets[i];
        test = target_square + mb_direction;
        if (test == kingside_mb_square)
            available &= 2;
        else if (test == queenside_mb_square)
            available &= 1;
        }

    //Pawns
    pawns = *(all_pieces[1^colour] + 1);
    while (pawns) {
        piece_square = BITSCAN(pawns) - 1;
        pawns ^= (U64)1 << piece_square;
        if (colour) {
            if (S_FILE(piece_square)) {
                target_square = to_mailbox[piece_square - 9];
                if (target_square == kingside_mb_square)
                    available &= 2;
                else if (target_square == queenside_mb_square)
                    available &= 1;
                else if (target_square == king_square)
                    available = 0;
            }
            if (S_FILE(piece_square) != 7) {
                target_square = to_mailbox[piece_square - 7];
                if (target_square == kingside_mb_square)
                    available &= 2;
                else if (target_square == queenside_mb_square)
                    available &= 1;
                else if (target_square == king_square)
                    available = 0;
            }
        } else {
            if (S_FILE(piece_square)) {
                target_square = to_mailbox[piece_square + 7];
                if (target_square == kingside_mb_square)
                    available &= 2;
                else if (target_square == queenside_mb_square)
                    available &= 1;
                else if (target_square == king_square)
                    available = 0;
            }
            if (S_FILE(piece_square) != 7) {
                target_square = to_mailbox[piece_square + 9];
                if (target_square == kingside_mb_square)
                    available &= 2;
                else if (target_square == queenside_mb_square)
                    available &= 1;
                else if (target_square == king_square)
                    available = 0;
            }
        }
    }
    //Knight
    knights = *(all_pieces[1^colour] + 2);
    while (knights) {
        piece_square = BITSCAN(knights) - 1;
        knights ^= (U64)1 << piece_square;
        for (i=0;i<8;i++) {
            target_square = to_mailbox[piece_square];
            mb_direction = knight_mb_offsets[i];
            test = target_square + mb_direction;
            if (test == kingside_mb_square)
                available &= 2;
            else if (test == queenside_mb_square)
                available &= 1;
            else if (test == king_square)
                available = 0;
        }
    }

    //bishops
    bishops = *(all_pieces[1^colour] + 3) | *(all_pieces[1^colour] + 5);
    while (bishops) {
        piece_square = BITSCAN(bishops) - 1;
        bishops ^= (U64)1 << piece_square;
        start_square = piece_square;
        for (i=0;i<4;i++) {
            piece_square = start_square;
            target_square = to_mailbox[start_square];
            mb_direction = bishop_mb_offsets[i];
            true_direction = bishop_true_offsets[i];
            while (1) {
                target_square += mb_direction;
                test = mailbox[target_square];
                if (test != OFF) {
                    piece_square += true_direction;
                    target_piece = occupied[piece_square];
                    if (!target_piece) {
                        if (target_square == kingside_mb_square)
                            available &= 2;
                        else if (target_square == queenside_mb_square)
                            available &= 1;
                    } else {
                        if (target_square == king_square)
                            available = 0;
                        break;
                    }
                } else {
                    break; }
            }
        }
    }

    //rooks
    rooks = *(all_pieces[1^colour] + 4) | *(all_pieces[1^colour] + 5);
    while (rooks) {
        piece_square = BITSCAN(rooks) - 1;
        rooks ^= (U64)1 << piece_square;
        start_square = piece_square;
        for (i=0;i<4;i++) {
            piece_square = start_square;
            target_square = to_mailbox[start_square];
            mb_direction = rook_mb_offsets[i];
            true_direction = rook_true_offsets[i];
            while (1) {
                target_square += mb_direction;
                test = mailbox[target_square];
                if (test != OFF) {
                    piece_square += true_direction;
                    target_piece = occupied[piece_square];
                    if (!target_piece) {
                        if (target_square == kingside_mb_square)
                            available &= 2;
                        else if (target_square == queenside_mb_square)
                            available &= 1;
                    } else {
                        if (target_square == king_square)
                            available = 0;
                        break;
                    }
                } else 
                    break;
            }
        }
    }

    return available;
}

// Move printing routine
void print_moves(int index) {
    int count=0;
    MOV move;
    while ((move = allmoves[index][count++]))
        move_decode(move);
}


/* Make move and unmake move routines
Where the mutations to the occupation and state variables comes in. 
-- A move counter to count the number of moves that have been made
-- 5 arrays to store:
- Last move made
- Old halfmove clock
- Old Castling Rights
- Old En Passent information. (Storing a 0 means ep is unavailable, as 0 is never a valid ep target)
- Piece captured (Storing a 0 means we captured an empty square i.e. quiet move)
We don't store the move number as it isn't really of interest.
Arrays will contain garbage at some point, but we will never actually access these values thanks to our move counter */
int num_moves = 0;
MOV moves_made[6];
int captures[6];
char old_halfmove[6];
char old_castling[6];
char old_ep[6];

//Update halfmove clock before move is made
void update_halfmove(int move_from, int move_to, int move_flags) {
    if ((move_flags & 4) || occupied[move_from] == (colour ? 7 : 5))
        halfmove_clock = 0;
    else
        halfmove_clock++;
}

//Update castling rights before move is made
void update_castling(int move_from, int move_to, int move_flags) {
    if (castling_rights) {
        if ((move_flags == 2) || (move_flags == 3))
            castling_rights &= (colour ? 3 : 12);
        else if (!move_flags) {
            if (colour) {
                if (move_from == 56)
                    castling_rights &= 7;
                else if (move_from == 63)
                    castling_rights &= 11;
                else if (move_from == 60)
                    castling_rights &= 3;
            } else {
                if (move_from == 0)
                    castling_rights &= 13;
                else if (move_from == 7)
                    castling_rights &= 14;
                else if (move_from == 4)
                    castling_rights &= 12;
            }
        }
        else if (move_flags & 4) {
            if (colour) {
                if (move_from == 56)
                    castling_rights &= 7;
                else if (move_from == 63)
                    castling_rights &= 11;
                else if (move_from == 60)
                    castling_rights &= 3;
                
                if (move_to == 0)
                    castling_rights &= 13;
                else if (move_to == 7)
                    castling_rights &= 14;
            } else {
                if (move_from == 0)
                    castling_rights &= 13;
                else if (move_from == 7)
                    castling_rights &= 14;
                else if (move_from == 4)
                    castling_rights &= 12;
                
                if (move_to == 56)
                    castling_rights &= 7;
                else if (move_to == 63)
                    castling_rights &= 11;
            }
        }
    }
}

//Make-move routine
void make_move(MOV move) {
    int move_from, move_to, move_flags, from_piece, to_piece, promoting_piece;
    move_from = MOVE_SRC(move);
    move_to = MOVE_TRG(move);
    move_flags = MOVE_FLG(move);
    int captured;

    //Save information
    moves_made[num_moves] = move;
    captures[num_moves] = occupied[move_to];
    old_halfmove[num_moves] = halfmove_clock;
    old_castling[num_moves] = castling_rights;
    old_ep[num_moves] = (ep_legal ? ep_square : 0);

    //First updates
    ep_legal = 0;
    ep_square = 0;
    update_castling(move_from, move_to, move_flags);
    update_halfmove(move_from, move_to, move_flags);

    if (!move_flags || (move_flags == 1)) { //Quiet move
        if (move_flags == 1) {  //Update en passent information
            ep_legal = 1;
            ep_square = move_to + (colour ? 8 : -8);
        }
        from_piece = occupied[move_from];
        occupied[move_to] = from_piece;
        occupied[move_from] = 0;
        *(all_pieces[colour] + (from_piece >> 2)) ^= (U64)1 << move_from;
        *(all_pieces[colour] + (from_piece >> 2)) ^= (U64)1 << move_to;

    } else if ((move_flags == 2) || (move_flags == 3)) {
        if (move_flags == 2) {
            if (colour) {
                occupied[60] = 0;
                occupied[63] = 0;
                occupied[61] = 19;
                occupied[62] = 3;
                b_pieces[0] ^= 0x5000000000000000;
                b_pieces[4] ^= 0xA000000000000000;
            } else {
                occupied[4] = 0;
                occupied[7] = 0;
                occupied[5] = 17;
                occupied[6] = 1;
                w_pieces[0] ^= 0x0000000000000050;
                w_pieces[4] ^= 0x00000000000000A0;
            }
        } else {
            if (colour) {
                occupied[60] = 0;
                occupied[56] = 0;
                occupied[59] = 19;
                occupied[58] = 3;
                b_pieces[0] ^= 0x1400000000000000;
                b_pieces[4] ^= 0x0900000000000000;
            } else {
                occupied[4] = 0;
                occupied[0] = 0;
                occupied[3] = 17;
                occupied[2] = 1;
                w_pieces[0] ^= 0x0000000000000014;
                w_pieces[4] ^= 0x0000000000000009;
            }
        }

    } else if (move_flags == 4) {   //Standard Capture
        from_piece = occupied[move_from];
        to_piece = occupied[move_to];
        occupied[move_to] = from_piece;
        occupied[move_from] = 0;
        *(all_pieces[colour] + (from_piece >> 2)) ^= (U64)1 << move_from;
        *(all_pieces[colour] + (from_piece >> 2)) ^= (U64)1 << move_to;
        *(all_pieces[1^colour] + (to_piece >> 2)) ^= (U64)1 << move_to;
    } else if (move_flags == 5) {   //En Passent  
        if (colour) {
            occupied[move_to] = 7;
            occupied[move_from] = 0;
            occupied[move_to + 8] = 0;
            b_pieces[1] ^= (U64)1 << move_from;
            b_pieces[1] ^= (U64)1 << move_to;
            w_pieces[1] ^= (U64)1 << (move_to + 8);
        } else {
            occupied[move_to] = 5;
            occupied[move_from] = 0;
            occupied[move_to - 8] = 0;
            w_pieces[1] ^= (U64)1 << move_from;
            w_pieces[1] ^= (U64)1 << move_to;
            b_pieces[1] ^= (U64)1 << (move_to - 8);
        }
    } else { //Promotion
        to_piece = occupied[move_to];
        promoting_piece = (((move_flags & 3) + 2) << 2) + (colour ? 3 : 1);
        if (move_flags & 4) {   //Capture promotion
            if (colour) {
                occupied[move_from] = 0;
                occupied[move_to] = promoting_piece;
                b_pieces[1] ^= (U64)1 << move_from;
                b_pieces[2 + (move_flags & 3)] ^= (U64)1 << move_to;
                w_pieces[to_piece >> 2] ^= (U64)1 << move_to;
            } else {
                occupied[move_from] = 0;
                occupied[move_to] = promoting_piece;
                w_pieces[1] ^= (U64)1 << move_from;
                w_pieces[2 + (move_flags & 3)] ^= (U64)1 << move_to;
                b_pieces[to_piece >> 2] ^= (U64)1 << move_to;
            }
        } else {//Quiet promotion
            if (colour) {
                occupied[move_from] = 0;
                occupied[move_to] = promoting_piece;
                b_pieces[1] ^= (U64)1 << move_from;
                b_pieces[2 + (move_flags & 3)] ^= (U64)1 << move_to;
            } else {
                occupied[move_from] = 0;
                occupied[move_to] = promoting_piece;
                w_pieces[1] ^= (U64)1 << move_from;
                w_pieces[2 + (move_flags & 3)] ^= (U64)1 << move_to;
            }
        }
    }
    //Flip side and increment counter
    colour ^= 1;
    num_moves++;
}

//Unmake-move function
void unmake_move() {
    int move_from, move_to, move_flags, moved_piece,removed_piece, promoting_piece;
    MOV last_move;
    
    //Restore information and gather move information
    last_move = moves_made[--num_moves];
    move_from = MOVE_SRC(last_move);
    move_to = MOVE_TRG(last_move);
    move_flags = MOVE_FLG(last_move);
    colour ^= 1;
    ep_legal = 0;
    ep_square = 0;
    if (old_ep[num_moves]) {
        ep_legal = 1;
        ep_square = old_ep[num_moves];
    }
    halfmove_clock = old_halfmove[num_moves];
    castling_rights = old_castling[num_moves];

    if (!move_flags || (move_flags == 1)) { //Quiet move
        moved_piece = occupied[move_to];
        occupied[move_to] = 0;
        occupied[move_from] = moved_piece;
        *(all_pieces[colour] + (moved_piece >> 2)) ^= (U64)1 << move_from;
        *(all_pieces[colour] + (moved_piece >> 2)) ^= (U64)1 << move_to;

    } else if ((move_flags == 2) || (move_flags == 3)) {
        if (move_flags == 2) {
            if (colour) {
                occupied[61] = 0;
                occupied[62] = 0;
                occupied[63] = 19;
                occupied[60] = 3;
                b_pieces[0] ^= 0x5000000000000000;
                b_pieces[4] ^= 0xA000000000000000;
            } else {
                occupied[5] = 0;
                occupied[6] = 0;
                occupied[7] = 17;
                occupied[4] = 1;
                w_pieces[0] ^= 0x0000000000000050;
                w_pieces[4] ^= 0x00000000000000A0;
            }
        } else {
            if (colour) {
                occupied[59] = 0;
                occupied[58] = 0;
                occupied[56] = 19;
                occupied[60] = 3;
                b_pieces[0] ^= 0x1400000000000000;
                b_pieces[4] ^= 0x0900000000000000;
            } else {
                occupied[3] = 0;
                occupied[2] = 0;
                occupied[0] = 17;
                occupied[4] = 1;
                w_pieces[0] ^= 0x0000000000000014;
                w_pieces[4] ^= 0x0000000000000009;
            }
        }

    } else if (move_flags == 4) {   //Standard Capture
        removed_piece = captures[num_moves];
        moved_piece = occupied[move_to];
        occupied[move_to] = removed_piece;
        occupied[move_from] = moved_piece;
        *(all_pieces[colour] + (moved_piece >> 2)) ^= (U64)1 << move_from;
        *(all_pieces[colour] + (moved_piece >> 2)) ^= (U64)1 << move_to;
        *(all_pieces[1^colour] + (removed_piece >> 2)) ^= (U64)1 << move_to;
    } else if (move_flags == 5) {   //En Passent  
        if (colour) {
            occupied[move_to] = 0;
            occupied[move_from] = 7;
            occupied[move_to + 8] = 5;
            b_pieces[1] ^= (U64)1 << move_from;
            b_pieces[1] ^= (U64)1 << move_to;
            w_pieces[1] ^= (U64)1 << (move_to + 8);
        } else {
            occupied[move_to] = 0;
            occupied[move_from] = 5;
            occupied[move_to - 8] = 7;
            w_pieces[1] ^= (U64)1 << move_from;
            w_pieces[1] ^= (U64)1 << move_to;
            b_pieces[1] ^= (U64)1 << (move_to - 8);
        }
    } else { //Promotion
        removed_piece = captures[num_moves];
        promoting_piece = (((move_flags & 3) + 2) << 2) + (colour ? 3 : 1);
        if (move_flags & 4) {   //Capture promotion
            if (colour) {
                occupied[move_from] = 7;
                occupied[move_to] = removed_piece;
                b_pieces[1] ^= (U64)1 << move_from;
                b_pieces[2 + (move_flags & 3)] ^= (U64)1 << move_to;
                w_pieces[removed_piece >> 2] ^= (U64)1 << move_to;
            } else {
                occupied[move_from] = 5;
                occupied[move_to] = removed_piece;
                w_pieces[1] ^= (U64)1 << move_from;
                w_pieces[2 + (move_flags & 3)] ^= (U64)1 << move_to;
                b_pieces[removed_piece >> 2] ^= (U64)1 << move_to;
            }
        } else {//Quiet promotion
            if (colour) {
                occupied[move_from] = 7;
                occupied[move_to] = 0;
                b_pieces[1] ^= (U64)1 << move_from;
                b_pieces[2 + (move_flags & 3)] ^= (U64)1 << move_to;
            } else {
                occupied[move_from] = 5;
                occupied[move_to] = 0;
                w_pieces[1] ^= (U64)1 << move_from;
                w_pieces[2 + (move_flags & 3)] ^= (U64)1 << move_to;
            }
        }
    }
}

/* Evaluation: Exact same evaluation function as before.
Will only give piece square tables from white perspective. For black perspective,
We take the square index and flip its rank bits (XOR with 0x38) */
int material_counts[6] = {20000, 100, 300, 320, 500, 900};
int pawn_mg_table[64] = 
{0,0,0,0,0,0,0,0,
5,5,10,-10,-10,10,10,5,
0,0,-5,0,0,-10,0,5,
0,0,10,30,30,0,0,0,
0,0,15,35,35,15,0,0,
5,10,20,40,40,20,10,5,
50,50,50,50,50,50,50,50,
0,0,0,0,0,0,0,0};
int pawn_eg_table[64] =
{0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
5,5,5,5,5,5,5,5,
10,10,10,10,10,10,10,10,
20,20,20,20,20,20,20,20,
30,30,30,30,30,30,30,30,
50,50,50,50,50,50,50,50,
0,0,0,0,0,0,0,0};
int king_mg_table[64] = 
{5,10,0,-15,-10,-15,10,5,
0,0,-10,-20,-20,-15,0,0,
-10,-10,-15,-35,-35,-15,-10,-10,
-20,-20,-30,-40,-40,-30,-20,-20,
-30,-30,-40,-50,-50,-40,-30,-30,
-30,-40,-40,-60,-60,-40,-40,-30,
-30,-60,-80,-100,-100,-80,-60,-30,
-30,-60,-80,-100,-100,-80,-60,-30};
int king_eg_table[64] = 
{-50,-40,-30,-20,-20,-30,-40,-50,
-30,-15,0,0,0,0,-15,-30,
-10,0,10,25,25,10,0,-10,
0,10,30,50,50,30,10,0,
0,10,30,50,50,30,10,0,
-10,0,10,25,25,10,0,-10,
-30,-15,0,0,0,0,-15,-30,
-50,-40,-30,-20,-20,-30,-40,-50};
int knight_table[64] = 
{-30,-5,-5,0,0,-5,-5,-30,
-10,0,0,0,0,0,0,-10,
-5,0,8,5,5,8,0,-5,
0,5,5,12,12,5,5,0,
0,5,5,12,12,5,5,0,
-5,0,8,5,5,8,0,-5,
-10,0,0,0,0,0,0,-10,
-30,-5,-5,0,0,-5,-5,-30};
int bishop_table[64] = 
{0,0,-5,0,0,-5,0,0,
0,7,0,2,2,0,7,0,
0,0,3,5,5,3,0,0,
2,5,7,12,12,7,5,2,
2,5,7,12,12,7,5,2,
0,0,3,5,5,3,0,0,
0,7,0,2,2,0,7,0,
0,0,-5,0,0,-5,0,0};
int rook_table[64] = 
{-10,-10,0,15,15,0,-10,-10,
0,0,0,10,10,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
10,10,10,10,10,10,10,10,
20,20,20,30,30,20,20,20,
0,0,10,10,10,10,0,0};
int queen_table[64] =
{-10,-10,-5,5,5,-5,-10,-10,
-5,0,5,0,0,5,0,-5,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
-5,0,5,0,0,5,0,-5,
-10,-10,-5,5,5,-5,-10,-10};

int *middlegame_psts[6] = {king_mg_table, pawn_mg_table,knight_table,
 bishop_table,rook_table,queen_table};
 int *endgame_psts[6] = {king_eg_table, pawn_eg_table,knight_table,
 bishop_table,rook_table,queen_table};

 //Evaluation function
 int evaluate() {
     int score, w_material, b_material, piece_square;
     int w_undeveloped, b_undeveloped;
     int *current_table;
     int i,endgame;
     U64 current_pieces;
     //Material counts
     for (w_material=0,b_material=0,i=2;i<6;i++) {
         w_material += POPCOUNT(w_pieces[i]) * material_counts[i];
         b_material += POPCOUNT(b_pieces[i]) * material_counts[i];
     }
     score = w_material - b_material;
     endgame = (w_material + b_material) <= 2800;
     score += (POPCOUNT(w_pieces[1]) - POPCOUNT(b_pieces[1])) * 100;
     //Positional scores
     if (endgame) {
         for (i=0; i<6; i++) {
             current_table = endgame_psts[i];
             current_pieces = w_pieces[i];
             while (current_pieces) {
                piece_square = BITSCAN(current_pieces) - 1;
                current_pieces ^= (U64)1 << (piece_square); 
                score += *(current_table + piece_square); }
            current_pieces = b_pieces[i];
            while (current_pieces) {
                piece_square = BITSCAN(current_pieces) - 1;
                current_pieces ^= (U64)1 << (piece_square); 
                score -= *(current_table + (0x38 ^ piece_square)); }
         }
     } else {
         for (i=0; i<6; i++) {
             current_table = middlegame_psts[i];
             current_pieces = w_pieces[i];
             while (current_pieces) {
                piece_square = BITSCAN(current_pieces) - 1;
                current_pieces ^= (U64)1 << (piece_square); 
                score += *(current_table + piece_square); }
            current_pieces = b_pieces[i];
            while (current_pieces) {
                piece_square = BITSCAN(current_pieces) - 1;
                current_pieces ^= (U64)1 << (piece_square); 
                score -= *(current_table + (0x38 ^ piece_square)); }
         }
     }
     //Development
     w_undeveloped = POPCOUNT((w_pieces[2] | w_pieces[3]) & 0x00000000000000FF);
     b_undeveloped = POPCOUNT((b_pieces[2] | b_pieces[3]) & 0xFF00000000000000);
     score += (b_undeveloped - w_undeveloped) * 33;
     
     //Pawn central control
     score += POPCOUNT(w_pieces[1] & 0x0000001818000000) * 20;
     score -= POPCOUNT(b_pieces[1] & 0x0000001818000000) * 20;

     //Trade queens if up in material
     if ((!w_pieces[5]) && (!b_pieces[5])) {
         score += w_material - b_material;
     }
     return (colour ? -score : score);
 }

 /* Search: Basic Negamax search routine */
 MOV best_move_found = 0;
 int search(int depth) {
    int score;
    int best_score = -10000;
    MOV best_move = 0;
    int k;
    int idx = 0;
    MOV m;

    if (!depth)
        return evaluate();
    k = generate_moves(4 - depth);
    if (k)
        return 10000;
    else {
        while ((m = allmoves[4-depth][idx++])) {
            make_move(m);
            score = -search(depth - 1);
            unmake_move();
            if (score >= best_score) {
                best_score = score;
                best_move = m;
            }
        }
        best_move_found = best_move;
        return best_score;
    }
 }

 //Reset the move counter so that search can start afresh
 void reset_counter() {num_moves = 0;}

 // Move parsing
int parse_move() {
    char c, user_input[6];
    int k, from_rank, from_file, to_rank, to_file, promotion;
    int idx=0;
    MOV encoded_move, test_move, m;
    k = generate_moves(0);
    if (fgets(user_input, 7, stdin) != NULL) {
        if (user_input[0] != '\n') {
            from_file = user_input[0] - 'a';
            from_rank = user_input[1] - '1';
            to_file = user_input[2] - 'a';
            to_rank = user_input[3] - '1';
            encoded_move = (from_rank << 9) | (from_file << 6) | (to_rank << 3) | to_file;
            while ((m = allmoves[0][idx++])) {
                test_move = m & 0xFFF;
                if (test_move == encoded_move) {
                    if (m & 0x8000) {//Promotion
                        switch(user_input[4]) {
                            case 'N':
                                promotion = 0;
                                break;
                            case 'B':
                                promotion = 1;
                                break;
                            case 'R':
                                promotion = 2;
                                break;
                            case 'Q':
                                promotion = 3;
                                break;
                            default:
                                return 0;
                        }
                        m &= 0xCFFF;
                        m |= (promotion << 12);
                    }
                    make_move(m);
                    print_board();
                    return 1;
                }     } } };
    return 0;
}

// Move getting routine
void get_move() {
    int r;
    while (1) {
        printf("\nMove:");
        r = parse_move();
        if (r)
            break;
    }
    reset_counter();
}

// Driver loop
void driver() {
    int colour_response, load_response;
    int engine_evaluation;
    printf("Choose starting options:\n");
    colour_response = (getc(stdin) == 'w') ? 0 : 1;
    load_response = (getc(stdin) == 'y') ? 1 : 0;
    if (load_response) {
        getc(stdin);
        reinitialize();
    }
        summarise_position();
    print_board();
    if (colour_response == colour)
        get_move();
    while (1) {
        engine_evaluation = search(4);
        move_decode(best_move_found);
        make_move(best_move_found);
        print_board();
        reset_counter();

        get_move();
    }
}

/* To do:
--Further optimizations
--Go for checkmate instead of stalemate! */


int main() {
    driver();
}
    