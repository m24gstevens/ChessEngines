"""attack_sets_12x10.py: attack routines for 12x10 board

General setup is as follows: An attack set is simply an array of integers corresponding
to the squares under attack."""

ENLARGED_LEGAL_BOARD = [0]*20 + [0,1,1,1,1,1,1,1,1,0]*8 + [0]*20
KNIGHT_ENLARGED_OFFSETS = [-21, -19, -12, -8, 8, 12, 19, 21]
KING_ENLARGED_OFFSETS = [-11,-10,-9,-1,1,9,10,11]

def index_array_to_120(array):
    """Takes an array of square indexes and puts them in a 12x10 array"""
    board_array = [[0]*10, [0]*10, [0]*10, [0]*10, [0]*10, [0]*10, [0]*10, [0]*10, [0]*10, [0]*10, [0]*10, [0]*10]
    for index in array:
        row = index // 10
        col = index % 10
        board_array[row][col] = 1
    return board_array

def move_encode_12x10(flags, from_square, to_square):
    """Move encoding to be used in move generation"""
    return (flags << 14) | (from_square << 7) | to_square


def bishop_attacks_encoded(bishop, occupied, enemy_occupied):
    """Calculates the attack set for a bishop standing at the bishop_square
    position. Takes all occupied squares and enemy occupied
    squares as arugments to determine when a bishop attacks an enemy piece.

    Attack squares are encoded as moves, used in move generation"""
    from_code = bishop << 7
    capture_code = 4 << 14 | bishop << 7
    potential_captures = []
    quiet = []
    ne_direction_square = bishop
    while True:
        ne_direction_square += 11
        if occupied[ne_direction_square]:
            potential_captures.append(ne_direction_square)
            break
        quiet.append(from_code | ne_direction_square) 
        
    nw_direction_square = bishop
    while True:
        nw_direction_square += 9
        if occupied[nw_direction_square]:
            potential_captures.append(nw_direction_square)
            break
        quiet.append(from_code | nw_direction_square) 

    se_direction_square = bishop
    while True:
        se_direction_square -= 9
        if occupied[se_direction_square]:
            potential_captures.append(se_direction_square)
            break
        quiet.append(from_code | se_direction_square)
        
    sw_direction_square = bishop
    while True:
        sw_direction_square -= 11
        if occupied[sw_direction_square]:
            potential_captures.append(sw_direction_square)
            break
        quiet.append(from_code | sw_direction_square) 

    captures = [capture_code | s for s in potential_captures if enemy_occupied[s]]
    return quiet, captures

def rook_attacks_encoded(rook, occupied, enemy_occupied):
    """Calculates the attack set for a rook standing at the given position.
    Takes all occupied squares and enemy occupied
    squares as arugments to determine when a bishop attacks an enemy piece.

    Attacked squares are encoded as moves to be used later"""
    from_code = rook << 7
    capture_code = 4 << 14 | rook << 7
    potential_captures = []
    quiet = []
    n_direction_square = rook
    while True:
        n_direction_square += 10
        if occupied[n_direction_square]:
            potential_captures.append(n_direction_square)
            break
        quiet.append(from_code | n_direction_square)

    s_direction_square = rook    
    while True:
        s_direction_square -= 10
        if occupied[s_direction_square]:
            potential_captures.append(s_direction_square)
            break
        quiet.append(from_code | s_direction_square)

    e_direction_square = rook
    while True:
        e_direction_square += 1
        if occupied[e_direction_square]:
            potential_captures.append(e_direction_square)
            break
        quiet.append(from_code | e_direction_square)

    w_direction_square = rook
    while True:
        w_direction_square -= 1
        if occupied[w_direction_square]:
            potential_captures.append(w_direction_square)
            break
        quiet.append(from_code | w_direction_square)

    captures = [capture_code | s for s in potential_captures if enemy_occupied[s]]
    return quiet, captures


def knight_attacks_encoded(knight, occupied, enemy_occupied):
    """Calculates the attack set for a knight standing at the knight
    position. Takes all squares occupied by own colour as an argument to cancel attacks on
    own pieces.
    
    Procedure is lengthy to reduce endless tests.
    Attacked squares are encoded as moves to be used later"""
    attacked_squares = []
    capture_code = 4 << 14 | knight << 7
    quiet_code = knight << 7
    rank = (knight - 21) // 10
    file = (knight - 21) % 10
    if file > 1:
        if file < 6:    #unrestricted left jump
            if rank > 1:
                attacked_squares = [knight - 21, knight - 19, knight - 12, knight - 8]
                if rank < 7:
                    attacked_squares += [knight + 8, knight + 12]
                    if rank < 6: # below seventh rank
                        attacked_squares += [knight + 19, knight + 21]
            else:
                if rank == 1:    #second rank
                    attacked_squares = [knight - 12, knight - 8]
                attacked_squares += [knight + 8, knight + 12, knight + 19, knight + 21]
                    
        elif file==6:    #g-file
            if rank > 1:
                attacked_squares = [knight - 21, knight - 19, knight - 12]
                if rank < 7:
                    attacked_squares.append(knight + 8)
                    if rank < 6:
                        attacked_squares += [knight + 19, knight + 21]                    
            else:
                if rank == 1:    #second rank
                    attacked_squares = [knight - 12]
                attacked_squares += [knight + 8, knight + 19, knight + 21]
            
        else:           #h-file
            if rank < 2:
                if rank == 1:    #second rank
                    attacked_squares = [knight - 12]
                attacked_squares += [knight + 8, knight + 19]
            else:
                attacked_squares = [knight - 21, knight - 12]
                if rank < 7:
                    attacked_squares.append(knight + 8)
                    if rank < 6:
                        attacked_squares.append(knight + 19)
                    
    else:    #unrestricted right jump
        if file==1:  #b-file
            if rank > 1:
                attacked_squares = [knight - 21, knight - 19, knight - 8]
                if rank != 7:
                    attacked_squares.append(knight + 12)
                    if rank != 6:   #below seventh rank
                        attacked_squares += [knight + 19, knight + 21]
            else:
                if rank == 1:    #second rank
                    attacked_squares = [knight - 8]
                attacked_squares += [knight + 12, knight + 19, knight + 21]

        else:       #a-file
            if rank > 1:
                attacked_squares = [knight - 19, knight - 8]
                if rank != 7:
                    attacked_squares.append(knight + 12)
                    if rank != 6:   #below seventh rank
                        attacked_squares.append(knight + 21)
            else:
                if rank == 1:    #second rank
                    attacked_squares = [knight - 8]
                attacked_squares += [knight + 12, knight + 21]

    #Don't attack own                    
    quiet = [quiet_code | square for square in attacked_squares if occupied[square] == 0 ]
    captures = [capture_code | square for square in attacked_squares if enemy_occupied[square]]
    return quiet, captures


def king_attacks_encoded(king, occupied, enemy_occupied):
    """Calculates the attack set for a king standing at the given position.
    Takes all occupied squares and enemy occupied
    squares as arugments to determine when a king attacks an enemy piece.

    Lengthy procedure to reduce total number of conditional tests
    Attack squares encoded as moves"""
    attacked_squares = []
    capture_code = 4 << 14 | king << 7
    quiet_code = king << 7
    rank = (king - 21) // 10
    file = (king - 21) % 10
    if rank != 0:    #Beyond first rank
        if file != 0:    #not a-file
            if file != 7:  #not h-file
                attacked_squares = [king - 11, king - 10, king - 9, king - 1, king + 1]
                if rank != 7:   #not last rank
                    attacked_squares += [king + 9, king + 10, king + 11]
            else:           #h-file
                attacked_squares = [king - 11, king - 10, king -1]
                if rank != 7:   #not last rank again
                    attacked_squares += [king + 9, king + 10]
        else:               #a-file
            attacked_squares = [king - 10, king - 9, king + 1]
            if rank != 7:
                attacked_squares += [king + 10, king + 11]
    else:               #1st rank
        if file == 0:    #a-file
            attacked_squares = [king + 1, king + 10, king + 11]
        elif file == 7:  #h-file
            attacked_squares = [king - 1, king + 9, king + 10]
        else:
            attacked_squares = [king - 1, king + 1, king + 9, king + 10, king + 11]

    quiet = [quiet_code | square for square in attacked_squares if occupied[square] == 0 ]
    captures = [capture_code | square for square in attacked_squares if enemy_occupied[square]]
    return quiet, captures

def pawn_moves_improved(pawns, colour, occupied, enemy_occupied):
    """Generates the attack and push set for all pawns in the pawn array.
    returns an array of encoded moves to be used for move generation"""
    if colour:  #black
        pushes = []
        captures = []
        promotions = []
        advances = [pawn - 10 for pawn in pawns if not occupied[pawn - 10]]
        for square in advances:
            if square > 30:
                pushes.append((square + 10) << 7 | square)
            else:
                promotions = [(8 + i) << 14 | (square + 10) << 7 | square for i in range(4)]

        sw_captures = [pawn - 11 for pawn in pawns if enemy_occupied[pawn - 11]]
        for square in sw_captures:
            if square > 30:
                captures.append(1 << 16 | (square + 11) << 7 | square)
            else:
                promotions += [(12 + i) << 14 | (square + 11) << 7 | square for i in range(4)]

        se_captures = [pawn - 9 for pawn in pawns if enemy_occupied[pawn - 9]]
        for square in se_captures:
            if square > 30:
                captures.append(1 << 16 | (square + 9) << 7 | square)
            else:
                promotions += [(12 + i) << 14 | (square + 9) << 7 | square for i in range(4)]
                
        return pushes, captures, promotions

    else:       #white
        pushes = []
        captures = []
        promotions = []
        advances = [pawn + 10 for pawn in pawns if not occupied[pawn + 10]]
        for square in advances:
            if square < 90:
                pushes.append((square - 10) << 7 | square)
            else:
                promotions = [(8 + i) << 14 | (square - 10) << 7 | square for i in range(4)]

        sw_captures = [pawn + 11 for pawn in pawns if enemy_occupied[pawn + 11]]
        for square in sw_captures:
            if square < 90:
                captures.append(1 << 16 | (square - 11) << 7 | square)
            else:
                promotions += [(12 + i) << 14 | (square - 11) << 7 | square for i in range(4)]

        se_captures = [pawn + 9 for pawn in pawns if enemy_occupied[pawn + 9]]
        for square in se_captures:
            if square < 90:
                captures.append(1 << 16 | (square - 9) << 7 | square)
            else:
                promotions += [(12 + i) << 14 | (square - 9) << 7 | square for i in range(4)]
                
        return pushes, captures, promotions
