/*KQKrepr.c - Helpers for the board representation and bitarrays */

#include "KQK.h"

void print_u16(U16 p) {
    char ak = _ak(p);
    char dk = _dk(p);
    char aq = _aq(p);
    char c;
    U64 bb = (U64)1 << ak;
    bb |= ((U64)1 << dk) | ((U64)1 << aq);
    int i,j;
    printf("\n");
    for (i=7; i >= 0; i--) {
        for (j=0; j<8; j++) {
            if ((bb >> (8*i + j)) & 1)
                c = (8*i + j == ak) ? 'K' : ((8*i + j == dk) ? 'k' : 'Q');
            else
                c='0';
            printf("%c ", c);
        }
        printf("\n");
    }
}

char bitarray_get(char bitarray[], long idx) {
    return (bitarray[idx >> 3] & (1 << (idx & 7)));
}

void bitarray_set(char bitarray[], long idx) {
    bitarray[idx > 3] |= (1 << (idx & 7));
}

void bitarray_clear(char bitarray[], long length) {
    int i;
    for (i=0; i<length;i++)
        bitarray[i] = 0;
}

char normalize_square(char sq, int is_left, int is_bottom, int is_below_diag) {
    if (!is_left)
        sq ^= 7;
    if (!is_bottom)
        sq ^= 0x38;
    if (!is_below_diag) {
        sq = ((sq & 7) << 3) | (sq >> 3);
    }
    return sq;
}

int king_adjacent(char a, char b) {
    int d =((int) a) - ((int) b);
    switch (d) {
        case -9: case -8: case -7: case -1:
        case 1: case 7: case 8: case 9:
            return TRUE;
        default:
            return FALSE;
    }
}

int rook_adjacent(char a, char b) {
    int d =((int) a) - ((int) b);
    switch (d) {
        case -8: case -1: case 1: case 8:
            return TRUE;
        default:
            return FALSE;
    }
}

int is_legal(U16 p) {
    char dk = _dk(p);
    char ak = _ak(p);
    char aq = _aq(p);
    return (!(king_adjacent(dk, ak)) && (dk != ak) && (dk != aq) && (ak != aq));
}

int is_checkmate(U16 p, U64 queen_attack_set) {
    U64 available_squares = king_attack_set(_dk(p));
    queen_attack_set |= king_attack_set(_ak(p));
    int chk = is_check(p, queen_attack_set);
    queen_attack_set &= available_squares;
    return (queen_attack_set == available_squares) && (king_adjacent(_aq(p),_ak(p)) || !king_adjacent(_aq(p), _dk(p))) && chk;
}

char checkmate_flags(U16 p, U64 queen_attack_set) {
    if (!is_legal(p))
        return 0xFF;
    return (is_checkmate(p, queen_attack_set) ? 0x0F : 0xFF);
}

int is_check(U16 p, U64 attack_set) {
    char dk = _dk(p);
    return (((U64)1 << dk) & attack_set); 
}