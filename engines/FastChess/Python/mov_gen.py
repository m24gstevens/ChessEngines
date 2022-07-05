"""mov_gen_12x10.py: Generating and encoding moves given a board_state. 12x10
version"""

"""Moves are encoded by a 16-bit integer in the following way:
{4 flag bits}{6 from bits}{6 to bits} (in binary)
-4 flag bits generate the special moves: First bit is Promotion, Second is
    Capture, third and fourth are special bits. Encoding are as follows:
    -0000: Quiet move
    -0001: Double pawn push
    -0010: Kingside Castle
    -0011: Queenside Castle
    -0100: Capture
    -0101: En-Passent
    -1000: Knight promotion
    -1001: Bishop promotion
    -1010: Rook promotion
    -1011: Queen promotion
    -1100: Knight promotion with capture etc.
-6 from bits generate the square index the piece moves from
-6 to bits generate the square index the piece moves to"""

#How much pieces offset on a 10  by 12 board when moving 
KING_ENLARGED_OFFSETS = [-11,-10,-9,-1,1,9,10,11]
ROOK_ENLARGED_OFFSETS = [-10, -1, 1, 10]
BISHOP_ENLARGED_OFFSETS = [-11,-9,9,11]
KNIGHT_ENLARGED_OFFSETS = [-21, -19, -12, -8, 8, 12, 19, 21]
PAWN_ENLARGED_OFFSETS = [9,11]
LEGAL_BOARD = [0]*20 + [0,1,1,1,1,1,1,1,1,0]*8 + [0]*20

from attack_sets import *
from board_rep import *
import copy

def move_encode_12x10(flags, from_square, to_square):
    """Move encoding procedure"""
    return (flags << 14) | (from_square << 7) | to_square

def move_decode_12x10(code):
    """Debugging function for 12x10"""
    from_square = (code & 0b11111110000000) >> 7
    to_square = code & 0b1111111
    from_string = index_to_name_12x10(from_square)
    to_string = index_to_name_12x10(to_square)
    print(f"{from_string} to {to_string}")

def is_square_attacked_12x10(position, colour, square, occupied):
    """Determines whether the given colour attacks a given square."""
    colour_pieces = position[colour]
    queens = colour_pieces[5]
    for queen in queens:
        for offset in KING_ENLARGED_OFFSETS:
            target = queen
            while True:
                target += offset
                if target == square:
                    return True
                else:
                    if occupied[target]:
                        break

    rooks = colour_pieces[4]
    for rook in rooks:
        for offset in ROOK_ENLARGED_OFFSETS:
            target = rook
            while True:
                target += offset
                if target == square:
                    return True
                else:
                    if occupied[target]:
                        break
                    
    bishops = colour_pieces[3]
    for bishop in bishops:
        for offset in BISHOP_ENLARGED_OFFSETS:
            target = bishop
            while True:
                target += offset
                if target == square:
                    return True
                else:
                    if occupied[target]:
                        break

    knights = colour_pieces[2]
    for knight in knights:
        for offset in KNIGHT_ENLARGED_OFFSETS:
            target = knight + offset
            if target == square:
                return True

    pawns = colour_pieces[1]
    if colour:
        for pawn in pawns:
            for offset in PAWN_ENLARGED_OFFSETS:
                target = pawn - offset
                if target == square:
                    return True
    else:
        for pawn in pawns:
            for offset in PAWN_ENLARGED_OFFSETS:
                target = pawn + offset
                if target == square:
                    return True
                
    king = colour_pieces[0][0]
    for offset in KING_ENLARGED_OFFSETS:
        target = king + offset
        if target == square:
            return True
    

    return False

def is_square_attacked_excluding(excluded, position, colour, square, occupied):
    """Alternative function, ignoring any attacker on the excluded square"""
    colour_pieces = position[colour]
    queens = colour_pieces[5]
    for queen in queens:
        if queen != excluded:
            for offset in KING_ENLARGED_OFFSETS:
                target = queen
                while True:
                    target += offset
                    if target == square:
                        return True
                    else:
                        if occupied[target]:
                            break

    rooks = colour_pieces[4]
    for rook in rooks:
        if rook != excluded:
            for offset in ROOK_ENLARGED_OFFSETS:
                target = rook
                while True:
                    target += offset
                    if target == square:
                        return True
                    else:
                        if occupied[target]:
                            break
                    
    bishops = colour_pieces[3]
    for bishop in bishops:
        if bishop != excluded:
            for offset in BISHOP_ENLARGED_OFFSETS:
                target = bishop
                while True:
                    target += offset
                    if target == square:
                        return True
                    else:
                        if occupied[target]:
                            break

    knights = colour_pieces[2]
    for knight in knights:
        if knight != excluded:
            for offset in KNIGHT_ENLARGED_OFFSETS:
                target = knight + offset
                if target == square:
                    return True

    pawns = colour_pieces[1]
    if colour:
        for pawn in pawns:
            if pawn != excluded:
                for offset in PAWN_ENLARGED_OFFSETS:
                    target = pawn - offset
                    if target == square:
                        return True
    else:
        for pawn in pawns:
            if pawn != excluded:
                for offset in PAWN_ENLARGED_OFFSETS:
                    target = pawn + offset
                    if target == square:
                        return True
                
    king = colour_pieces[0][0]
    if king != excluded:
        for offset in KING_ENLARGED_OFFSETS:
            target = king + offset
            if target == square:
                return True

    return False

def attacked_squares_12x10(position, colour, occupied):
    """Determines whether the given colour attacks a given square,
    by calculating all attacked squares. Value in the array = number of times
    attacked"""
    attacked = [[]]*120
    colour_pieces = position[colour]
    queens = colour_pieces[5]
    for queen in queens:
        for offset in KING_ENLARGED_OFFSETS:
            target = queen
            while True:
                target += offset
                attacked[target] = attacked[target] + [queen]
                if occupied[target]:
                    break

    rooks = colour_pieces[4]
    for rook in rooks:
        for offset in ROOK_ENLARGED_OFFSETS:
            target = rook
            while True:
                target += offset
                attacked[target] = attacked[target] + [rook]
                if occupied[target]:
                    break
                    
    bishops = colour_pieces[3]
    for bishop in bishops:
        for offset in BISHOP_ENLARGED_OFFSETS:
            target = bishop
            while True:
                target += offset
                attacked[target] = attacked[target] + [bishop]
                if occupied[target]:
                    break

    knights = colour_pieces[2]
    for knight in knights:
        for offset in KNIGHT_ENLARGED_OFFSETS:
            target = knight + offset
            attacked[target] = attacked[target] + [knight]

    king = colour_pieces[0][0]
    for offset in KING_ENLARGED_OFFSETS:
        attacked[king + offset] = attacked[king + offset] + [king]

    pawns = colour_pieces[1]
    if colour:
        for pawn in pawns:
            for offset in PAWN_ENLARGED_OFFSETS:
                attacked[pawn - offset] = attacked[pawn - offset] + [pawn]
    else:
        for pawn in pawns:
            for offset in PAWN_ENLARGED_OFFSETS:
               attacked[pawn + offset] = attacked[pawn + offset] + [pawn]
               
    return attacked

def attacked_squares_no_king(position, colour, occupied):
    """Avoids the king"""
    attacked = [[]]*120
    colour_pieces = position[colour]
    queens = colour_pieces[5]
    for queen in queens:
        for offset in KING_ENLARGED_OFFSETS:
            target = queen
            while True:
                target += offset
                attacked[target] = attacked[target] + [queen]
                if occupied[target]:
                    break

    rooks = colour_pieces[4]
    for rook in rooks:
        for offset in ROOK_ENLARGED_OFFSETS:
            target = rook
            while True:
                target += offset
                attacked[target] = attacked[target] + [rook]
                if occupied[target]:
                    break
                    
    bishops = colour_pieces[3]
    for bishop in bishops:
        for offset in BISHOP_ENLARGED_OFFSETS:
            target = bishop
            while True:
                target += offset
                attacked[target] = attacked[target] + [bishop]
                if occupied[target]:
                    break

    knights = colour_pieces[2]
    for knight in knights:
        for offset in KNIGHT_ENLARGED_OFFSETS:
            target = knight + offset
            attacked[target] = attacked[target] + [knight]

    pawns = colour_pieces[1]
    if colour:
        for pawn in pawns:
            for offset in PAWN_ENLARGED_OFFSETS:
                attacked[pawn - offset] = attacked[pawn - offset] + [pawn]
    else:
        for pawn in pawns:
            for offset in PAWN_ENLARGED_OFFSETS:
               attacked[pawn + offset] = attacked[pawn + offset] + [pawn]
               
    return attacked

def movable_squares_12x10(position, colour, occupied):
    """Determines whether the given colour can move to a given square. Just
    like the attacking version, except we exclude king moves and pawn
    captures, including pawn pushes. Excludes all captures"""
    movable = [[]]*120
    colour_pieces = position[colour]
    queens = colour_pieces[5]
    for queen in queens:
        for offset in KING_ENLARGED_OFFSETS:
            target = queen
            while True:
                target += offset
                if occupied[target]:
                    break
                movable[target] = movable[target] + [queen]

    rooks = colour_pieces[4]
    for rook in rooks:
        for offset in ROOK_ENLARGED_OFFSETS:
            target = rook
            while True:
                target += offset
                if occupied[target]:
                    break
                movable[target] = movable[target] + [rook]
                    
    bishops = colour_pieces[3]
    for bishop in bishops:
        for offset in BISHOP_ENLARGED_OFFSETS:
            target = bishop
            while True:
                target += offset
                if occupied[target]:
                    break
                movable[target] = movable[target] + [bishop]

    knights = colour_pieces[2]
    for knight in knights:
        for offset in KNIGHT_ENLARGED_OFFSETS:
            target = knight + offset
            if not occupied[target]:
                movable[target] = movable[target] + [knight]

    pawns = colour_pieces[1]
    if colour:
        for pawn in pawns:
            if not occupied[pawn - 10]:
                movable[pawn - 10] = movable[pawn - 10] + [pawn]
                if not occupied[pawn - 20]:
                    movable[pawn - 20] = movable[pawn - 20] + [pawn]
    else:
        for pawn in pawns:
            if not occupied[pawn + 10]:
                movable[pawn + 10] = movable[pawn + 10] + [pawn]
                if not occupied[pawn + 20]:
                    movable[pawn + 20] = movable[pawn + 20] + [pawn]

    return movable

def pinned_pieces(position, own_occupied, enemy_occupied, occupied, occupying_pieces):
    """Determines which pieces (squares) are pinned by an enemy piece"""
    colour = position[2][0]
    enemy_pieces = position[1^colour]
    #print(colour)
    #print(position)
    #print(position[colour][0])
    king = position[colour][0][0]
    pins = {}
    for offset in BISHOP_ENLARGED_OFFSETS:
        target = king
        while True:
            target += offset
            if occupied[target]:
                if own_occupied[target]:
                    potential_pinned = target
                    while True:
                        target += offset
                        if occupied[target]:
                            if enemy_occupied[target]:
                                if occupying_pieces[target] == 3 or occupying_pieces[target] == 5:
                                    pins[potential_pinned] = offset
                        break
                    break
                else:
                    break

    for offset in ROOK_ENLARGED_OFFSETS:
        target = king
        while True:
            target += offset
            if occupied[target]:
                if own_occupied[target]:
                    potential_pinned = target
                    while True:
                        target += offset
                        if occupied[target]:
                            if enemy_occupied[target]:
                                if occupying_pieces[target] == 4 or occupying_pieces[target] == 5:
                                    pins[potential_pinned] = offset
                        break
                else:
                    break
                
    return pins

def en_passent_12x10(colour, ep_target, own_occupied):
    """Calculates all en-passent captures."""
    if ep_target:
        pawn_squares = []
        if colour:
            if own_occupied[ep_target + 9]:
                pawn_squares = [ep_target + 9]
            if own_occupied[ep_target + 11]:
                pawn_squares += [ep_target + 11]
        else:
            if own_occupied[ep_target - 9]:
                pawn_squares = [ep_target - 9]
            if own_occupied[ep_target - 11]:
                pawn_squares += [ep_target - 11]
        #The squares we can capture e.p. from are the occupied squares
        #which a pawn of the opposite colour standing at e.p. square would
        #attack
        return [move_encode_12x10(5, s, ep_square) for s in pawn_squares]
    return []

def en_passent_legal(position, king, colour, ep_target, own_occupied, occupied, occupying_pieces):
    """Calculates all en-passent captures."""
    if ep_target:
        pawn_squares = []
        if colour:
            if own_occupied[ep_target + 9] and occupying_pieces[ep_target + 9] == 1:
                pawn_squares = [ep_target + 9]
            if own_occupied[ep_target + 11] and occupying_pieces[ep_target + 11] == 1:
                pawn_squares += [ep_target + 11]
        else:
            if own_occupied[ep_target - 9] and occupying_pieces[ep_target - 9] == 1:
                pawn_squares = [ep_target - 9]
            if own_occupied[ep_target - 11] and occupying_pieces[ep_target -11] == 1:
                pawn_squares += [ep_target - 11]
        #The squares we can capture e.p. from are the occupied squares
        #which a pawn of the opposite colour standing at e.p. square would
        #attack
        test_occupied = copy.copy(occupied)
        ep = []
        for pawn in pawn_squares:
            ep_taken = ep_target + (10 if colour else -10)
            test_occupied[ep_target] = 1
            test_occupied[ep_taken] = 0
            test_occupied[pawn] = 0
            if not is_square_attacked_excluding(ep_taken, position, 1^colour, king, test_occupied):
                ep.append(5 << 14 | pawn << 7 | ep_target)
            test_occupied[pawn] = 1
            test_occupied[ep_target] = 0
            test_occupied[ep_taken] = 1

        return ep
    return []
            

def double_pushes_12x10(pawns, colour, occupied):
    """Double pawn pushes for all pawns"""
    if colour:  #black
        seventh_rank = [pawn for pawn in pawns if (pawn - 21) // 10 == 6]
        return [move_encode_12x10(1,pawn, pawn - 20) for pawn in seventh_rank
                if (not occupied[pawn - 10]) and (not occupied[pawn - 20])]
    else:  #white
        second_rank = [pawn for pawn in pawns if (pawn - 21) // 10 == 1]
        return [move_encode_12x10(1,pawn, pawn + 20) for pawn in second_rank
                if (not occupied[pawn + 10]) and (not occupied[pawn + 20])]

def double_pushes_legal(pawns, colour, occupied, pins):
    """Legal double pawn pushes"""
    if colour:  #black
        seventh_rank = [pawn for pawn in pawns if (pawn - 21) // 10 == 6]
        return [move_encode_12x10(1,pawn, pawn - 20) for pawn in seventh_rank
                if ((not occupied[pawn - 10])
                    and (not occupied[pawn - 20])
                    and (pins.get(pawn,10) == 10))]
    else:  #white
        second_rank = [pawn for pawn in pawns if (pawn - 21) // 10 == 1]
        return [move_encode_12x10(1,pawn, pawn + 20) for pawn in second_rank
                if ((not occupied[pawn + 10])
                    and (not occupied[pawn + 20])
                    and (pins.get(pawn, -10) == -10))]

def castling_12x10(colour, rights, own_occupied):
    """Generates all (maybe illegal) castling moves"""
    kingside = rights & 1
    queenside = rights & 2
    castling_moves = []
    rank = 7 if colour else 0
    king = 21 + (rank * 10) + 4
    if kingside:
        if not (own_occupied[king + 1]
                or own_occupied[king + 2]):
            castling_moves.append(move_encode_12x10(2, king, king + 2))
    if queenside:
        if not (own_occupied[king - 1]
                or own_occupied[king - 2]
                or own_occupied[king - 3]):
            castling_moves.append(move_encode_12x10(3, king, king - 3))
    return castling_moves

def castling_legal(colour, rights, occupied, enemy_occupied, threatened):
    """Generates all legal castling moves"""
    kingside = rights & 1
    queenside = rights & 2
    castling_moves = []
    rank = 7 if colour else 0
    king = 21 + (rank * 10) + 4
    if kingside:
        if not (occupied[king + 1]
                or occupied[king + 2]
                or threatened[king + 1]
                or threatened[king + 2]
                or enemy_occupied[king + 3]):
            castling_moves.append(move_encode_12x10(2, king, king + 2))
    if queenside:
        if not (occupied[king - 1]
                or occupied[king - 2]
                or occupied[king - 3]
                or threatened[king - 1]
                or threatened[king - 2]
                or enemy_occupied[king - 4]):
            castling_moves.append(move_encode_12x10(3, king, king - 2))
    return castling_moves

def pseudo_legal_12x10_improved(position, own_occupied, enemy_occupied, occupied):
    """Takes precalculated values of occupied squares"""
    other_states = position[2]
    colour = other_states[0]
    castling_rights = other_states[1 + colour]
    ep_target = other_states[3]
    own_pieces = position[colour]

    quiet = []
    captures = []
    special = []

    king = own_pieces[0][0]
    k_quiet, k_captures = king_attacks_encoded(king, occupied, enemy_occupied)
    quiet += k_quiet
    captures += k_captures
            
    pawns = own_pieces[1]        
    pawn_advance, pawn_capture, pawn_promote = pawn_moves_improved(pawns, colour, occupied, enemy_occupied)
    quiet += pawn_advance
    captures += pawn_capture
    special += pawn_promote

    special += en_passent_12x10(colour, ep_target, own_occupied)
    special += double_pushes_12x10(pawns, colour, occupied)

    special += castling_12x10(colour, position, castling_rights, own_occupied, occupied)

    for knight in own_pieces[2]:
        n_quiet, n_captures = knight_attacks_encoded(knight, occupied, enemy_occupied)
        quiet += n_quiet
        captures += n_captures

    for bishop in own_pieces[3] + own_pieces[5]:
        b_quiet, b_captures = bishop_attacks_encoded(bishop, occupied, enemy_occupied)
        quiet += b_quiet
        captures += b_captures

    for rook in own_pieces[4] + own_pieces[5]:
        r_quiet, r_captures = rook_attacks_encoded(rook, occupied, enemy_occupied)
        quiet += r_quiet
        captures += r_captures

    return quiet, captures, special

def pseudo_legal_no_proc(position, own_occupied, enemy_occupied, occupied):
    """Replaces procedure calls with procedure body. Very lengthy"""
    other_states = position[2]
    colour = other_states[0]
    castling_rights = other_states[1 + colour]
    ep_target = other_states[3]
    own_pieces = position[colour]
    
    threatened = attacked_squares_12x10(position, 1^colour, occupied)
    quiet = []
    captures = []
    special = []

    king = own_pieces[0][0]
    k_attacked_squares = []
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
            
    pawns = own_pieces[1]
    if colour:  #black
        seventh_rank = [pawn for pawn in pawns if (pawn - 21) // 10 == 6]
        special += [1 << 14 | pawn <<7 | pawn - 20 for pawn in seventh_rank
                if (not occupied[pawn - 10]) and (not occupied[pawn - 20])]
    
        advances = [pawn - 10 for pawn in pawns if not occupied[pawn - 10]]
        for square in advances:
            if square > 30:
                quiet.append((square + 10) << 7 | square)
            else:
                special += [(8 + i) << 14 | (square + 10) << 7 | square for i in range(4)]

        sw_captures = [pawn - 11 for pawn in pawns if enemy_occupied[pawn - 11]]
        for square in sw_captures:
            if square > 30:
                captures.append(1 << 16 | (square + 11) << 7 | square)
            else:
                captures += [(12 + i) << 14 | (square + 11) << 7 | square for i in range(4)]

        se_captures = [pawn - 9 for pawn in pawns if enemy_occupied[pawn - 9]]
        for square in se_captures:
            if square > 30:
                captures.append(1 << 16 | (square + 9) << 7 | square)
            else:
                captures += [(12 + i) << 14 | (square + 9) << 7 | square for i in range(4)]

    else:       #white
        second_rank = [pawn for pawn in pawns if (pawn - 21) // 10 == 1]
        special += [1 << 14 | pawn << 7 | pawn + 20 for pawn in second_rank
                if (not occupied[pawn + 10]) and (not occupied[pawn + 20])]

        advances = [pawn + 10 for pawn in pawns if not occupied[pawn + 10]]
        for square in advances:
            if square < 90:
                quiet.append((square - 10) << 7 | square)
            else:
                special += [(8 + i) << 14 | (square - 10) << 7 | square for i in range(4)]

        sw_captures = [pawn + 11 for pawn in pawns if enemy_occupied[pawn + 11]]
        for square in sw_captures:
            if square < 90:
                captures.append(1 << 16 | (square - 11) << 7 | square)
            else:
                captures += [(12 + i) << 14 | (square - 11) << 7 | square for i in range(4)]

        se_captures = [pawn + 9 for pawn in pawns if enemy_occupied[pawn + 9]]
        for square in se_captures:
            if square < 90:
                captures.append(1 << 16 | (square - 9) << 7 | square)
            else:
                captures += [(12 + i) << 14 | (square - 9) << 7 | square for i in range(4)]

    special += en_passent_12x10(colour, ep_target, own_occupied)

    special += castling_12x10(colour, castling_rights, own_occupied)

    for knight in own_pieces[2]:
        n_attacked_squares = []
        capture_code = 4 << 14 | knight << 7
        quiet_code = knight << 7
        rank = (knight - 21) // 10
        file = (knight - 21) % 10
        if file > 1:
            if file < 6:    #unrestricted left jump
                if rank > 1:
                    n_attacked_squares = [knight - 21, knight - 19, knight - 12, knight - 8]
                    if rank < 7:
                        n_attacked_squares += [knight + 8, knight + 12]
                        if rank < 6: # below seventh rank
                            n_attacked_squares += [knight + 19, knight + 21]
                else:
                    if rank == 1:    #second rank
                        n_attacked_squares = [knight - 12, knight - 8]
                    n_attacked_squares += [knight + 8, knight + 12, knight + 19, knight + 21]
                    
            elif file==6:    #g-file
                if rank > 1:
                    n_attacked_squares = [knight - 21, knight - 19, knight - 12]
                    if rank < 7:
                        n_attacked_squares.append(knight + 8)
                        if rank < 6:
                            n_attacked_squares += [knight + 19, knight + 21]                    
                else:
                    if rank == 1:    #second rank
                        n_attacked_squares = [knight - 12]
                    n_attacked_squares += [knight + 8, knight + 19, knight + 21]
            
            else:           #h-file
                if rank < 2:
                    if rank == 1:    #second rank
                        n_attacked_squares = [knight - 12]
                    n_attacked_squares += [knight + 8, knight + 19]
                else:
                    n_attacked_squares = [knight - 21, knight - 12]
                    if rank < 7:
                        n_attacked_squares.append(knight + 8)
                        if rank < 6:
                            n_attacked_squares.append(knight + 19)
                    
        else:    #unrestricted right jump
            if file==1:  #b-file
                if rank > 1:
                    n_attacked_squares = [knight - 21, knight - 19, knight - 8]
                    if rank != 7:
                        n_attacked_squares.append(knight + 12)
                        if rank != 6:   #below seventh rank
                            n_attacked_squares += [knight + 19, knight + 21]
                else:
                    if rank == 1:    #second rank
                        n_attacked_squares = [knight - 8]
                    n_attacked_squares += [knight + 12, knight + 19, knight + 21]

            else:       #a-file
                if rank > 1:
                    n_attacked_squares = [knight - 19, knight - 8]
                    if rank != 7:
                        n_attacked_squares.append(knight + 12)
                        if rank != 6:   #below seventh rank
                            n_attacked_squares.append(knight + 21)
                else:
                    if rank == 1:    #second rank
                        n_attacked_squares = [knight - 8]
                    n_attacked_squares += [knight + 12, knight + 21]

        #Don't attack own                    
        quiet += [quiet_code | square for square in n_attacked_squares if occupied[square] == 0 ]
        captures += [capture_code | square for square in n_attacked_squares if enemy_occupied[square]]

    for bishop in own_pieces[3] + own_pieces[5]:
        from_code = bishop << 7
        capture_code = 4 << 14 | bishop << 7
        potential_captures = []
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

        captures += [capture_code | s for s in potential_captures if enemy_occupied[s]]

    for rook in own_pieces[4] + own_pieces[5]:
        from_code = rook << 7
        capture_code = 4 << 14 | rook << 7
        potential_captures = []
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

        captures += [capture_code | s for s in potential_captures if enemy_occupied[s]]

    return quiet, captures, special, 0

def legal_moves(position, own_occupied, enemy_occupied, occupied):
    """Generates all legal moves by modifying procedures."""
    quiet, captures, special = pseudo_legal_no_proc(position, own_occupied, enemy_occupied, occupied)
    true_quiet = []
    test_occupied = copy.copy(occupied)
    colour = position[2][0]
    king = position[colour][0]
    for move in quiet:
        from_square = (move & 0b11111110000000)>>7
        to_square = move & 0b1111111
        to_occupied = test_occupied[to_square]
        test_occupied[from_square] = 0
        test_occupied[to_square] = 1
        if not is_square_attacked_12x10(position, 1^colour, king, test_occupied):
            true_quiet.append(move)
        test_occupied[from_square] = 1
        test_occupied[to_square] = to_occupied
    true_captures = []
    for move in captures:
        from_square = (move & 0b11111110000000)>>7
        to_square = move & 0b1111111
        to_occupied = test_occupied[to_square]
        test_occupied[from_square] = 0
        test_occupied[to_square] = 1
        if not is_square_attacked_12x10_excluding(to_square, position, 1^colour, king, test_occupied):
            true_captures.append(move)
        test_occupied[from_square] = 1
        test_occupied[to_square] = to_occupied
    true_special = []
    for move in special:
        code = (move & 0b111100000000000000) >> 14
        from_square = (move & 0b11111110000000)>>7
        to_square = move & 0b1111111
        to_occupied = test_occupied[to_square]
        test_occupied[from_square] = 0
        test_occupied[to_square] = 1
        if code == 5:
            test_occupied[position[2][3] + (10 if position[2][0] else -10)] = 0
        if not is_square_attacked_12x10(position, 1^colour, king, test_occupied):
            true_quiet.append(move)
        test_occupied[from_square] = 1
        test_occupied[to_square] = to_occupied
        if code == 5:
            test_occupied[position[2][3] + (10 if position[2][0] else -10)] = 1


    return true_quiet, true_captures, true_special, 0

def legal_moves_improved(position, own_occupied, enemy_occupied, occupied, occupying_pieces):
    """Uses less brute force methods"""
    other_states = position[2]
    colour = other_states[0]
    castling_rights = other_states[1 + colour]
    ep_target = other_states[3]
    own_pieces = position[colour]
    
    threatened = attacked_squares_12x10(position, 1^colour, occupied)
    pins = pinned_pieces(position, own_occupied, enemy_occupied, occupied, occupying_pieces)
    moves = []

    king = own_pieces[0][0]
    if threatened[king]:
        return in_check_moves(position, own_occupied, enemy_occupied, occupied, occupying_pieces, threatened, pins)
    k_attacked_squares = []
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

    attacked_squares = [square for square in attacked_squares if not threatened[square]]
    moves += [quiet_code | square for square in attacked_squares if occupied[square] == 0 ]
    moves += [capture_code | square for square in attacked_squares if enemy_occupied[square]]

    pawns = own_pieces[1]
    pinned_pawns = [pawn for pawn in pawns if pins.get(pawn,0)]
    unpinned = [pawn for pawn in pawns if not pins.get(pawn, 0)]
    if colour:  #black
        seventh_rank = [pawn for pawn in unpinned if (pawn - 21) // 10 == 6]
        moves += [1 << 14 | pawn <<7 | pawn - 20 for pawn in seventh_rank
                if (not occupied[pawn - 10]) and (not occupied[pawn - 20])]
    
        advances = [pawn - 10 for pawn in unpinned if not occupied[pawn - 10]]
        for square in advances:
            if square > 30:
                moves.append((square + 10) << 7 | square)
            else:
                moves += [(8 + i) << 14 | (square + 10) << 7 | square for i in range(4)]

        sw_captures = [pawn - 11 for pawn in unpinned if enemy_occupied[pawn - 11]]
        for square in sw_captures:
            if square > 30:
                moves.append(1 << 16 | (square + 11) << 7 | square)
            else:
                moves += [(12 + i) << 14 | (square + 11) << 7 | square for i in range(4)]

        se_captures = [pawn - 9 for pawn in unpinned if enemy_occupied[pawn - 9]]
        for square in se_captures:
            if square > 30:
                moves.append(1 << 16 | (square + 9) << 7 | square)
            else:
                moves += [(12 + i) << 14 | (square + 9) << 7 | square for i in range(4)]

    else:       #white
        second_rank = [pawn for pawn in unpinned if (pawn - 21) // 10 == 1]
        moves += [1 << 14 | pawn << 7 | pawn + 20 for pawn in second_rank
                if (not occupied[pawn + 10]) and (not occupied[pawn + 20])]

        advances = [pawn + 10 for pawn in unpinned if not occupied[pawn + 10]]
        for square in advances:
            if square < 90:
                moves.append((square - 10) << 7 | square)
            else:
                moves += [(8 + i) << 14 | (square - 10) << 7 | square for i in range(4)]

        sw_captures = [pawn + 11 for pawn in unpinned if enemy_occupied[pawn + 11]]
        for square in sw_captures:
            if square < 90:
                moves.append(1 << 16 | (square - 11) << 7 | square)
            else:
                moves += [(12 + i) << 14 | (square - 11) << 7 | square for i in range(4)]

        se_captures = [pawn + 9 for pawn in unpinned if enemy_occupied[pawn + 9]]
        for square in se_captures:
            if square < 90:
                moves.append(1 << 16 | (square - 9) << 7 | square)
            else:
                moves += [(12 + i) << 14 | (square - 9) << 7 | square for i in range(4)]

    for pawn in pinned_pawns:
        offset = pins.get(pawn)
        if colour:
            if offset == -10:
                if not occupied[pawn - 10]:
                    moves.append(pawn << 7 | pawn - 10)
                    if not occupied[pawn - 20]:
                        moves.append(1 << 14 | pawn << 7 | pawn - 20)
            else:
                if offset in [-9, -11]:
                    if enemy_occupied[pawn + offset]:
                        moves.append(1 << 16 | pawn << 7 | pawn + offset)
        else:
            if offset == 10:
                if not occupied[pawn + 10]:
                    moves.append(pawn << 7 | pawn + 10)
                    if not occupied[pawn + 20]:
                        moves.append(1 << 14 | pawn << 7 | pawn + 20)
            else:
                if offset in [9, 11]:
                    if enemy_occupied[pawn + offset]:
                        moves.append(1 << 16 | pawn << 7 | pawn + offset)
        
        
    moves += en_passent_legal(position, king, colour, ep_target, own_occupied, occupied, occupying_pieces)

    moves += castling_legal(colour, castling_rights, occupied, enemy_occupied, threatened)

    for knight in own_pieces[2]:
        if pins.get(knight,0):
            continue
        else:
            n_attacked_squares = []
            capture_code = 4 << 14 | knight << 7
            quiet_code = knight << 7
            rank = (knight - 21) // 10
            file = (knight - 21) % 10
            if file > 1:
                if file < 6:    #unrestricted left jump
                    if rank > 1:
                        n_attacked_squares = [knight - 21, knight - 19, knight - 12, knight - 8]
                        if rank < 7:
                            n_attacked_squares += [knight + 8, knight + 12]
                            if rank < 6: # below seventh rank
                                n_attacked_squares += [knight + 19, knight + 21]
                    else:
                        if rank == 1:    #second rank
                            n_attacked_squares = [knight - 12, knight - 8]
                        n_attacked_squares += [knight + 8, knight + 12, knight + 19, knight + 21]
                        
                elif file==6:    #g-file
                    if rank > 1:
                        n_attacked_squares = [knight - 21, knight - 19, knight - 12]
                        if rank < 7:
                            n_attacked_squares.append(knight + 8)
                            if rank < 6:
                                n_attacked_squares += [knight + 19, knight + 21]                    
                    else:
                        if rank == 1:    #second rank
                            n_attacked_squares = [knight - 12]
                        n_attacked_squares += [knight + 8, knight + 19, knight + 21]
                
                else:           #h-file
                    if rank < 2:
                        if rank == 1:    #second rank
                            n_attacked_squares = [knight - 12]
                        n_attacked_squares += [knight + 8, knight + 19]
                    else:
                        n_attacked_squares = [knight - 21, knight - 12]
                        if rank < 7:
                            n_attacked_squares.append(knight + 8)
                            if rank < 6:
                                n_attacked_squares.append(knight + 19)
                        
            else:    #unrestricted right jump
                if file==1:  #b-file
                    if rank > 1:
                        n_attacked_squares = [knight - 21, knight - 19, knight - 8]
                        if rank != 7:
                            n_attacked_squares.append(knight + 12)
                            if rank != 6:   #below seventh rank
                                n_attacked_squares += [knight + 19, knight + 21]
                    else:
                        if rank == 1:    #second rank
                            n_attacked_squares = [knight - 8]
                        n_attacked_squares += [knight + 12, knight + 19, knight + 21]

                else:       #a-file
                    if rank > 1:
                        n_attacked_squares = [knight - 19, knight - 8]
                        if rank != 7:
                            n_attacked_squares.append(knight + 12)
                            if rank != 6:   #below seventh rank
                                n_attacked_squares.append(knight + 21)
                    else:
                        if rank == 1:    #second rank
                            n_attacked_squares = [knight - 8]
                        n_attacked_squares += [knight + 12, knight + 21]

            #Don't attack own                    
            moves += [quiet_code | square for square in n_attacked_squares if occupied[square] == 0 ]
            moves += [capture_code | square for square in n_attacked_squares if enemy_occupied[square]]

    for bishop in own_pieces[3] + own_pieces[5]:
        if pins.get(bishop,0):
            pin_direction_square = bishop
            offset = pins[bishop]
            if offset in BISHOP_ENLARGED_OFFSETS:
                while True:
                    pin_direction_square += offset
                    if occupied[pin_direction_square]:
                        if enemy_occupied[pin_direction_square]:
                            moves.append(4 << 14 | bishop << 7 | pin_direction_square)
                        break
                    else:
                        moves.append(bishop << 7 | pin_direction_square)
        else:
            from_code = bishop << 7
            capture_code = 4 << 14 | bishop << 7
            potential_captures = []
            ne_direction_square = bishop
            while True:
                ne_direction_square += 11
                if occupied[ne_direction_square]:
                    potential_captures.append(ne_direction_square)
                    break
                moves.append(from_code | ne_direction_square) 
            
            nw_direction_square = bishop
            while True:
                nw_direction_square += 9
                if occupied[nw_direction_square]:
                    potential_captures.append(nw_direction_square)
                    break
                moves.append(from_code | nw_direction_square) 

            se_direction_square = bishop
            while True:
                se_direction_square -= 9
                if occupied[se_direction_square]:
                    potential_captures.append(se_direction_square)
                    break
                moves.append(from_code | se_direction_square)
            
            sw_direction_square = bishop
            while True:
                sw_direction_square -= 11
                if occupied[sw_direction_square]:
                    potential_captures.append(sw_direction_square)
                    break
                moves.append(from_code | sw_direction_square) 

            moves += [capture_code | s for s in potential_captures if enemy_occupied[s]]

    for rook in own_pieces[4] + own_pieces[5]:
        if pins.get(rook,0):
            pin_direction_square = rook
            offset = pins[rook]
            if offset in ROOK_ENLARGED_OFFSETS:
                while True:
                    pin_direction_square += offset
                    if occupied[pin_direction_square]:
                        if enemy_occupied[pin_direction_square]:
                            moves.append(4 << 14 | rook << 7 | pin_direction_square)
                        break
                    else:
                        moves.append(rook << 7 | pin_direction_square)
        else:
            from_code = rook << 7
            capture_code = 4 << 14 | rook << 7
            potential_captures = []
            n_direction_square = rook
            while True:
                n_direction_square += 10
                if occupied[n_direction_square]:
                    potential_captures.append(n_direction_square)
                    break
                moves.append(from_code | n_direction_square)

            s_direction_square = rook    
            while True:
                s_direction_square -= 10
                if occupied[s_direction_square]:
                    potential_captures.append(s_direction_square)
                    break
                moves.append(from_code | s_direction_square)

            e_direction_square = rook
            while True:
                e_direction_square += 1
                if occupied[e_direction_square]:
                    potential_captures.append(e_direction_square)
                    break
                moves.append(from_code | e_direction_square)

            w_direction_square = rook
            while True:
                w_direction_square -= 1
                if occupied[w_direction_square]:
                    potential_captures.append(w_direction_square)
                    break
                moves.append(from_code | w_direction_square)

            moves += [capture_code | s for s in potential_captures if enemy_occupied[s]]            
 
    return moves, 0

def legal_king_moves(position, own_occupied, enemy_occupied, occupied, occupying_pieces, threatened):
    """Legal king moves that aren't castling"""
    colour = position[2][0]
    king = position[colour][0][0]
    k_attacked_squares = []
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

    attacked_squares = [square for square in attacked_squares if not threatened[square]]
    quiet = [quiet_code | square for square in attacked_squares if occupied[square] == 0 ]
    captures = [capture_code | square for square in attacked_squares if enemy_occupied[square]]

    test_occupied = copy.copy(occupied)
    true_quiet = []
    for move in quiet:
        from_square = king
        to_square = move & 0b1111111
        test_occupied[from_square] = 0
        test_occupied[to_square] = 1
        if not is_square_attacked_12x10(position, 1^colour, to_square, test_occupied):
            true_quiet.append(move)
        test_occupied[from_square] = 1
        test_occupied[to_square] = 0

    true_capture = []
    for move in captures:
        from_square = king
        to_square = move & 0b1111111
        test_occupied[from_square] = 0
        if not is_square_attacked_12x10(position, 1^colour, to_square, test_occupied):
            true_capture.append(move)
        test_occupied[from_square] = 1

    return true_quiet + true_capture, 1


def in_check_moves(position, own_occupied, enemy_occupied, occupied, occupying_pieces, threatened, pins):
    """Moves which escape the king from check"""
    colour = position[2][0]
    king = position[colour][0][0]
    if len(threatened[king]) > 1:
        #Double check - King moves only
        return legal_king_moves(position, own_occupied, enemy_occupied, occupied, occupying_pieces, threatened)
    else:
        attacking_square = threatened[king][0]
        attacking_piece = occupying_pieces[attacking_square]
        moves, checked = legal_king_moves(position, own_occupied, enemy_occupied, occupied, occupying_pieces, threatened)
        own_attacked_squares = attacked_squares_no_king(position, colour, occupied)
        movable_squares = movable_squares_12x10(position, colour, occupied)
        potential_capturers = own_attacked_squares[attacking_square]
        for pc in potential_capturers:
            if not pins.get(pc, 0):
                if occupying_pieces[pc] == 1 and ((colour and attacking_square < 30) or (not colour and attacking_square > 90)):
                    moves += [(12 + i) << 14 | pc << 7| attacking_square for i in range(4)]
                else:
                    moves.append(1 << 16 | pc << 7 | attacking_square)

        if attacking_piece in [1,2]:
            return moves, 1
        else:
            actual_offset = None
            if attacking_piece == 3:
                offset_set = BISHOP_ENLARGED_OFFSETS
            elif attacking_piece == 4:
                offset_set = ROOK_ENLARGED_OFFSETS
            else:
                offset_set = KING_ENLARGED_OFFSETS
            for offset in offset_set:
                target = attacking_square
                while True:
                    target += offset
                    if occupied[target]:
                        if king == target:
                            actual_offset = offset
                        break
            target = attacking_square
            while True:
                target += actual_offset
                if target == king:
                    break
                else:
                    potential_blockers = movable_squares[target]
                    for pb in potential_blockers:
                        if not pins.get(pb, 0):
                            if occupying_pieces[pb] == 1 and ((colour and target < 30) or (not colour and target > 90)):
                                moves += [(8 + i) << 14 | pb << 7 | target for i in range(4)]
                            else:
                                if occupying_pieces[pb] == 1 and abs(pb - target) == 20:
                                    moves.append(1 << 14 | pb << 7 | target)
                                else:
                                    moves.append( pb << 7 | target)
            return moves, 1
                
            
            
                
            
        

def main():
    test_fen = 'r2q1rk1/pp1bppbp/2np1np1/8/2BNP3/2N1BP2/PPPQ2PP/R3K2R w KQ - 5 10'
    test_position = fen_to_position_12x10(test_fen)
    print_piece_placements_12x10(test_position)
    own_occupied = colour_occupied_12x10(test_position, 0)
    enemy_occupied = colour_occupied_12x10(test_position, 1)
    occupied, occupying_pieces = all_occupied_12x10(test_position)
    t1 = time.process_time()
    for i in range(1):
        q,c,s = legal_moves_improved(test_position, own_occupied, enemy_occupied, occupied, occupying_pieces)
    t = time.process_time()
    print(en_passent_legal(test_position, 25, 0, 75, own_occupied, occupied))
    allmovs = q + c + s
    for mov in allmovs:
        move_decode_12x10(mov)
    print(len(allmovs))
    


if __name__ == "__main__":
    main()
