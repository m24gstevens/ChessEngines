#include "eval.h"

int material_score[12] = {10000,100,300,320,500,950,-10000,-100,-300,-320,-500,-950};

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

int king_pstable[64] = 
{0, 20, 40, -15, 0, -15, 40, 20,
-10,-10, -20, -30, -30, -20, -10, -10,
-40, -40, -40, -50, -50, -40, -40, -40,
-40, -40, -40, -50, -50, -40, -40, -40,
-40, -40, -40, -50, -50, -40, -40, -40,
-40, -40, -40, -50, -50, -40, -40, -40,
-40, -40, -40, -50, -50, -40, -40, -40,
-40, -40, -40, -50, -50, -40, -40, -40
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

int rook_pstable[64] =
{
    0,  0,  10,  20,  20,  10,  0,  0,
    0,  0,  10,  20,  20,  10,  0,  0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     50,   50,  50,  50,  50,  50,   50,   50,
     50,   50,   50,  50,  50,   50,   50,   50,

};

int evaluate(board_t* board) {
    int score,ct,sq,i;
    U64 bb;

    score = 0;
    for (int i=0;i<12;i++) {
        bb = board->bitboards[i];
        score += material_score[i]*popcount(bb);
        while (bb) {
            sq = bitscanForward(bb);
            POP_LS1B(bb);
            switch (i) {
                case K:
                    score += king_pstable[sq];
                    break;
                case P:
                    score += pawn_pstable[sq];
                    break;
                case N:
                    score += knight_pstable[sq];
                    break;
                case B:
                    score += bishop_pstable[sq];
                    break;
                case R:
                    score += rook_pstable[sq];
                case k:
                    score -= king_pstable[56^sq];
                    break;
                case p:
                    score -= pawn_pstable[56^sq];
                    break;
                case n:
                    score -= knight_pstable[56^sq];
                    break;
                case b:
                    score -= bishop_pstable[56^sq];
                    break;
                case r:
                    score -= rook_pstable[56^sq];
                default:
                    ;
            }
        }
    }
    return score*(1-2*board->side);
}