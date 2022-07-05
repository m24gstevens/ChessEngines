"""eval.py: Evaluation function for the position.
Utilises piece-square tables (PST) for positional evaluation.

Evaluation function inspired by Chess Programming Wiki"""

MATERIAL_COUNTS = [20000, 100, 300, 320, 500, 900]

W_PAWN_TABLE = [0]*30 + [0,5,5,10,-10,-10,10,10,5,0]
W_PAWN_TABLE += [0,0,0,-5,0,0,-10,0,5,0]
W_PAWN_TABLE += [0,0,0,10,30,30,0,0,0,0]
W_PAWN_TABLE += [0,0,0,15,35,35,15,0,0,0]
W_PAWN_TABLE += [0,5,10,20,40,40,20,10,5,0]
W_PAWN_TABLE += [0,50,50,50,50,50,50,50,50,0] + [0] * 30

W_PAWN_EG_TABLE = [0]*30 + [0,0,0,0,0,0,0,0,0,0]
W_PAWN_EG_TABLE += [0,5,5,5,5,5,5,5,5,0]
W_PAWN_EG_TABLE += [0,10,10,10,10,10,10,10,10,0]
W_PAWN_EG_TABLE += [0,20,20,20,20,20,20,20,20,0]
W_PAWN_EG_TABLE += [0,30,30,30,30,30,30,30,30,0]
W_PAWN_EG_TABLE += [0,50,50,50,50,50,50,50,50,0] + [0] * 30

B_PAWN_TABLE = [0]*30 + [0,50,50,50,50,50,50,50,50,0]
B_PAWN_TABLE += [0,10,15,20,40,40,20,15,10,0]
B_PAWN_TABLE += [0,0,5,15,25,25,15,5,0,0]
B_PAWN_TABLE += [0,0,0,10,20,20,0,0,0,0]
B_PAWN_TABLE += [0,0,0,-5,0,0,-10,0,5,0]
B_PAWN_TABLE += [0,5,5,10,-10,-10,10,10,5,0] + [0] * 30

B_PAWN_EG_TABLE = [0]*30 + [0,50,50,50,50,50,50,50,50,0]
B_PAWN_EG_TABLE += [0,30,30,30,30,30,30,30,30,0]
B_PAWN_EG_TABLE += [0,20,20,20,20,20,20,20,20,0]
B_PAWN_EG_TABLE += [0,10,10,10,10,10,10,10,10,0]
B_PAWN_EG_TABLE += [0,5,5,5,5,5,5,5,5,0]
B_PAWN_EG_TABLE += [0,0,0,0,0,0,0,0,0,0] + [0] * 30

W_KING_MG_TABLE = [0]*20 + [0,5,10,0,-15,-10,-15,10,5,0]
W_KING_MG_TABLE += [0,0,0,-10,-20,-20,-15,0,0,0]
W_KING_MG_TABLE += [0,-10,-10,-15,-25,-25,-15,-10,-10,0]
W_KING_MG_TABLE += [0,-20,-20,-30,-40,-40,-30,-20,-20,0]
W_KING_MG_TABLE += [0,-30,-30,-40,-50,-50,-40,-30,-30,0]
W_KING_MG_TABLE += [0,-30,-40,-40,-60,-60,-40,-40,-30,0]
W_KING_MG_TABLE += [0,-30,-60,-80,-100,-100,-80,-60,-30,0]
W_KING_MG_TABLE += [0,-30,-60,-80,-100,-100,-80,-60,-30,0] + [0]*20

B_KING_MG_TABLE = [0]*20 + [0,-30,-60,-80,-100,-100,-80,-60,-30,0]
B_KING_MG_TABLE += [0,-30,-60,-80,-100,-100,-80,-60,-30,0]
B_KING_MG_TABLE += [0,-30,-40,-40,-60,-60,-40,-40,-30,0]
B_KING_MG_TABLE += [0,-30,-30,-40,-50,-50,-40,-30,-30,0]
B_KING_MG_TABLE += [0,-20,-20,-30,-40,-40,-30,-20,-20,0]
B_KING_MG_TABLE += [0,-10,-10,-15,-25,-25,-15,-10,-10,0]
B_KING_MG_TABLE += [0,0,0,-10,-20,-20,-15,0,0,0]
B_KING_MG_TABLE += [0,5,10,0,-15,-10,-15,10,5,0] + [0]*20

KING_EG_TABLE = [0]*20 + [0,-50,-40,-30,-20,-20,-30,-40,-50,0]
KING_EG_TABLE += [0,-30,-15,0,0,0,0,-15,-30,0]
KING_EG_TABLE += [0,-10,0,10,25,25,10,0,-10,0]
KING_EG_TABLE += [0,0,10,30,50,50,30,10,0,0]
KING_EG_TABLE += [0,0,10,30,50,50,30,10,0,0]
KING_EG_TABLE += [0,-10,0,10,25,25,10,0,-10,0]
KING_EG_TABLE += [0,-30,-15,0,0,0,0,-15,-30,0]
KING_EG_TABLE += [0,-50,-40,-30,-20,-20,-30,-40,-50,0] + [0]*20

KNIGHT_TABLE = [0]*20 + [0,-30,-5,-5,0,0,-5,-5,-30,0]
KNIGHT_TABLE += [0, -10,0,0,0,0,0,0,-10,0]
KNIGHT_TABLE += [0,-5,0,8,5,5,8,0,-5,0]
KNIGHT_TABLE += [0,0,5,5,12,12,5,5,0,0]
KNIGHT_TABLE += [0,0,5,5,12,12,5,5,0,0]
KNIGHT_TABLE += [0,-5,0,8,5,5,8,0,-5,0]
KNIGHT_TABLE += [0, -10,0,0,0,0,0,0,-10,0]
KNIGHT_TABLE += [0,-30,-5,-5,0,0,-5,-5,-30,0] + [0]*20

BISHOP_TABLE = [0]*20 + [0,0,0,-5,0,0,-5,0,0,0]
BISHOP_TABLE += [0,0,7,0,2,2,0,7,0,0]
BISHOP_TABLE += [0,0,0,3,5,5,3,0,0,0]
BISHOP_TABLE += [0,2,5,7,12,12,7,5,2,0]
BISHOP_TABLE += [0,2,5,7,12,12,7,5,2,0]
BISHOP_TABLE += [0,0,0,3,5,5,3,0,0,0]
BISHOP_TABLE += [0,0,7,0,2,2,0,7,0,0]
BISHOP_TABLE += [0,0,0,-5,0,0,-5,0,0,0] + [0]*20

W_ROOK_TABLE = [0]*20 + [0,-10,-10,0,15,15,0,-10,-10,0]
W_ROOK_TABLE += [0,0,0,0,10,10,0,0,0,0]
W_ROOK_TABLE += [0]*30
W_ROOK_TABLE += [0,10,10,10,10,10,10,10,10,0]
W_ROOK_TABLE += [0,20,20,20,30,30,20,20,20,0]
W_ROOK_TABLE += [0,0,0,10,10,10,10,0,0,0] + [0]*20

B_ROOK_TABLE = [0]*20 + [0,0,0,10,10,10,10,0,0,0]
B_ROOK_TABLE += [0,20,20,20,30,30,20,20,20,0]
B_ROOK_TABLE += [0,10,10,10,10,10,10,10,10,0]
B_ROOK_TABLE += [0]*30
B_ROOK_TABLE += [0,0,0,0,10,10,0,0,0,0]
B_ROOK_TABLE += [0,-10,-10,0,15,15,0,-10,-10,0] + [0]*20

Q_TABLE = [0]*20 + [0,-10,-10,-5,0,0,-5,-10,-10,0]
Q_TABLE += [0,-5,0,5,0,0,5,0,-5,0]
Q_TABLE += [0,0,5,0,0,0,0,5,0,0]
Q_TABLE += [0,0,0,0,0,0,0,0,0,0]
Q_TABLE += [0,0,0,0,0,0,0,0,0,0]
Q_TABLE += [0,0,5,3,3,3,3,5,0,0]
Q_TABLE += [0,-5,0,5,0,0,5,0,-5,0]
Q_TABLE += [0,-10,-10,-5,0,0,-5,-10,-10,0] + [0]*20

MIDDLEGAME_PSTS = [[W_KING_MG_TABLE, W_PAWN_TABLE, KNIGHT_TABLE, BISHOP_TABLE, W_ROOK_TABLE, Q_TABLE],
                   [B_KING_MG_TABLE, B_PAWN_TABLE, KNIGHT_TABLE, BISHOP_TABLE, B_ROOK_TABLE, Q_TABLE]]
ENDGAME_PSTS = [[W_KING_MG_TABLE, W_PAWN_EG_TABLE, KNIGHT_TABLE, BISHOP_TABLE, W_ROOK_TABLE, Q_TABLE],
                [B_KING_MG_TABLE, B_PAWN_EG_TABLE, KNIGHT_TABLE, BISHOP_TABLE, B_ROOK_TABLE, Q_TABLE]]


def evaluate(position):
    """Static evaluation of a position. Returns a score relative to the side
    being evaluated"""
    score = 0
    w_material_count = 0
    b_material_count = 0
    for i in range(2,6):
        w_material_count += len(position[0][i]) * MATERIAL_COUNTS[i]
        b_material_count += len(position[1][i]) * MATERIAL_COUNTS[i]
    score = w_material_count - b_material_count
    endgame = (w_material_count + b_material_count) <= 2600
    score += (len(position[0][1]) - len(position[1][1])) * MATERIAL_COUNTS[1]
    if endgame:
        for i in range(6):
            w_pieces = position[0][i]
            b_pieces = position[1][i]
            for j in w_pieces:
                score += ENDGAME_PSTS[0][i][j]
            for j in b_pieces:
                score -= ENDGAME_PSTS[0][i][j]
    else:
        for i in range(6):
            w_pieces = position[0][i]
            b_pieces = position[1][i]
            for j in w_pieces:
                score += MIDDLEGAME_PSTS[0][i][j]
            for j in b_pieces:
                score -= MIDDLEGAME_PSTS[1][i][j]
                

    return (-score if position[2][0] else score)
    





