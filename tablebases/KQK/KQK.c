/* KQK.c - Generates the tablebase for KQ vs K positions */
#include "KQK.h"

/* parse_fen: Parses the FEN string of a position into our 16-bit board representation.
Returns bitmask of (side with queen, side to move, legal position boolean) where white=0, black=1 */
char parse_fen(char fen[], char *d, char *e, char *f) {
    int i=0;
    int j=0;
    char status=TRUE;
    char wk,bk,c;
    char q=0;
    int idx = 0;

    for (i=7; i>=0; i--) {
        for (j=0; (c = fen[idx++]) != '/' && c != ' ';) 
            switch (c) {
                case 'p': case 'P': case 'b': case 'B': case 'n': case 'N':
                case 'r': case 'R':
                    status=FALSE;
                    break;
                case 'k':
                    bk = 8*i + j++;
                    break;
                case 'K':
                    wk = 8*i + j++;
                    break;
                case 'q': case 'Q':
                    if (q & (1 << 7))
                        status = FALSE;
                    q |= (1 << 7) | (i << 3 | j);
                    q |= (isupper(c) ? 0 : (1 << 6));
                    break;
                default:
                    j += c - '0';
                    break;
            }
    }
    if (status) {
        *e = q & 0x3F;
        if (q & (1 << 6)) {
            *d = bk;
            *f = wk;
        } else {
            *d = wk;
            *f = bk;
        }
    }
    return status | (fen[idx] == 'b' ? 2 : 0) | (q & (1 << 7) ? 4 : 0);
}

/* to_board_repr: Given square indices of where attacking king and queen and defending queen are,
returns the U16 board representation used for lookup */
U16 to_board_repr(char ak, char aq, char dk) {
    U16 repr = 0;
    /* Put defending king in lower left corner */
    int is_left = ((dk & 7) < 4);
    int is_bottom = ((dk >> 3) < 4);
    int is_below_diag = ((dk & 7) >= (dk >> 3));
    ak = normalize_square(ak, is_left, is_bottom, is_below_diag);
    aq = normalize_square(aq, is_left, is_bottom, is_below_diag);
    dk = normalize_square(dk, is_left, is_bottom, is_below_diag);
    return ak | (aq << 6) | ((dk & 0xF) << 12);
}

/* Generating the tablebase */

int generate_tablebase() {
    /* initialization of attack sets */
    char all_positions[NUM_POSITIONS];
    char bitarray[NUM_POSITIONS >> 3];
    U16 i=NUM_POSITIONS - 1;
    char depth = 0;
    U16 idx;
    U16 depth_count;
    pos_list ancestors;
    char ms1bTable[0xFF];
    U64 rayAttacks[8][64];
    init_ray_attacks(rayAttacks);
    for (i=0;i<0xFF;i++)
        ms1bTable[i] = naive_ms1b(i);
    U64 queenAttackTable[64][64];
    gen_queen_attack_tables(rayAttacks, ms1bTable, queenAttackTable);

    do {
        all_positions[i] = checkmate_flags(i, queenAttackTable[_aq(i)][_ak(i)]);
    } while ((i--) != 0);
    for (i=0; i<NUM_POSITIONS; i++) {
        if (is_checkmate(i, queenAttackTable[_aq(i)][_ak(i)]) && is_legal(i)) {
            print_u16(i);
            printf("\n");
        }
    } 
    print_bb(queenAttackTable[5][36]);
    /* algorithm begins */
    while (depth < 0xF) {
        depth_count = 0;
        for (i=0; i<NUM_POSITIONS; i++) {
            if ((all_positions[i] & 0xF0) == (depth << 4)) {
                /* generate the white to move parent positions */
                depth_count++;
                gen_white_parents(i, queenAttackTable, &ancestors);
                for (idx = 0; idx < ancestors.length; idx++)
                    all_positions[ancestors.positions[idx]] ^= (~(depth + 1) & 0x0F);
            }
        }
        if (!depth_count)
            break;
        bitarray_clear(bitarray, NUM_POSITIONS >> 3);
        for (i=0; i<NUM_POSITIONS; i++) {
            if ((all_positions[i] & 0x0F) == (depth + 1) << 4) {
                /* generate the black to move parent positions */
                gen_black_ancestors(i,&ancestors);
                for (idx = 0; idx < ancestors.length; idx++)
                    bitarray_set(bitarray, ancestors.positions[idx]);
            }
        }
        for (i=0; i<NUM_POSITIONS; i++) {
            if (bitarray_get(bitarray, i)) {
                /* generate the white to move child positions
                If find none of sentinel depth, assign a depth */
                gen_white_children(i, queenAttackTable, &ancestors);
                for (idx = 0; idx < ancestors.length; idx++) {
                    if ((all_positions[ancestors.positions[idx]] & 0x0F) == 0x0F)
                        goto kill;
                }
                all_positions[i] ^= (~(depth + 1) & 0xF0);
                continue;
                kill:
                    continue;
            }
        }
        depth++;
    }
    create_file(all_positions, NUM_POSITIONS, "KQK.bin");
}

int main() {
    generate_tablebase();
    printf("Done");
}

/* Todo: Continue debugging. So far, the initialization is all correct. */