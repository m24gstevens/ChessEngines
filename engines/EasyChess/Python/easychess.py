"""main.py: Main script that will be run. Starts off initializing the global
data structures, defines make/unmake move, and gets onto the search"""

from board_rep import *
from mov_gen import *
from evaluater import *

W_QUEENSIDE_ROOK = 21
W_KING = 25
W_KINGSIDE_ROOK = 28
B_QUEENSIDE_ROOK = 91
B_KING = 95
B_KINGSIDE_ROOK = 98


position = fen_to_position_12x10("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
moves = [0]*10
num_moves = 0
captures = {}
irreversible = []
"""Irreversible is a list of tuples of (White Castling rights,
Black Castling rights, en-passent target square, halfmove clock)"""

w_occupied = colour_occupied_12x10(position, 0)
b_occupied = colour_occupied_12x10(position, 1)
occupied, occupying_pieces = all_occupied_12x10(position)

def reinitialize(fen):
    """Resets to the position described in the FEN argument with no history"""
    global position, moves, num_moves, captures, irreversible, w_occupied, b_occupied, occupied, occupying_pieces
    position = fen_to_position_12x10(fen)
    w_occupied = colour_occupied_12x10(position, 0)
    b_occupied = colour_occupied_12x10(position, 1)
    occupied, occupying_pieces = all_occupied_12x10(position)
    num_moves = 0
    captures = {}
    irreversible = []

def reset_variables():
    """Resets moves, num_moves, captures, irreversibles, occupied sets to get
    ready to search again"""
    global position, moves, num_moves, captures, irreversible, w_occupied, b_occupied, occupied, occupying_pieces
    num_moves = 0
    captures = {}
    irreversible = []

def move_info(move):
    """Given a move code, separates it into flags, from square and to square"""
    flags = move >> 14
    from_square = (move & 0b11111110000000) >> 7
    to_square = move & 0b1111111
    return flags, from_square, to_square

def update_halfmove(flags, from_square, to_square):
    """Updates the halfmove clock of the position"""
    global position
    if (flags & 4) or occupying_pieces[from_square] == 1:
        position[2][4] = 0

def update_castles(colour, flags, from_square, to_square):
    """Updates the castling state of the position"""
    global position
    if position[2][1] or position[2][2]:
        if flags in [2,3]:
            position[2][1 + colour] = 0
        elif flags == 0:
            if colour:
                if from_square == 95:
                    position[2][2] = 0
                elif from_square == B_KINGSIDE_ROOK:
                    position[2][2] &= 2
                elif from_square == B_QUEENSIDE_ROOK:
                    position[2][2] &= 1
            else:
                if from_square == 25:
                    position[2][1] = 0
                elif from_square == W_KINGSIDE_ROOK:
                    position[2][1] &= 2
                elif from_square == W_QUEENSIDE_ROOK:
                    position[2][1] &= 1
        elif flags & 4:
            if colour:
                if from_square == 95:
                    position[2][2] = 0
                elif from_square == B_KINGSIDE_ROOK:
                    position[2][2] &= 2
                elif from_square == B_QUEENSIDE_ROOK:
                    position[2][2] &= 1
                    
                if to_square == W_KINGSIDE_ROOK:
                    position[2][1] &= 2
                elif to_square == W_QUEENSIDE_ROOK:
                    position[2][1] &= 1
            else:
                if from_square == 25:
                    position[2][1] = 0
                elif from_square == W_KINGSIDE_ROOK:
                    position[2][1] &= 2
                elif from_square == W_QUEENSIDE_ROOK:
                    position[2][1] &= 1
                    
                if to_square == B_KINGSIDE_ROOK:
                    position[2][2] &= 2
                elif to_square == B_QUEENSIDE_ROOK:
                    position[2][2] &= 1

def make_move(move):
    """Assuming a legal move code as an input, updates the board representation
    accordingly"""
    global position, moves, num_moves, captures, irreversible, w_occupied, b_occupied, occupied, occupying_pieces
    #Save irreversible factors
    moves[num_moves] = move
    num_moves += 1
    irreversible.append((position[2][1], position[2][2], position[2][3], position[2][4]))
    #Decode the move
    colour = position[2][0]
    flags, from_square, to_square = move_info(move)
    #Update the other position factors (Castling, ep, halfmove, move, colour)
    position[2][3] = 0
    position[2][0] ^= 1
    if colour:
        position[2][5] += 1
    update_halfmove(flags, from_square, to_square)
    update_castles(colour, flags, from_square, to_square)
    if flags in [0,1]:  #quiet move
        if flags == 1:  #Double pawn push
            position[2][3] = to_square + (10 if colour else -10)
        #Update occupation sets
        occupied[from_square] = 0
        occupied[to_square] = 1
        if colour:
            b_occupied[from_square] = 0
            b_occupied[to_square] = 1
        else:
            w_occupied[from_square] = 0
            w_occupied[to_square] = 1
        piece_code = occupying_pieces[from_square]
        del occupying_pieces[from_square]
        occupying_pieces[to_square] = piece_code
        position[colour][piece_code].remove(from_square)
        position[colour][piece_code].append(to_square)
        return

    if flags in [2,3]:  #Castles
        if flags == 2:  #Kingside
            if colour:
                occupied[B_KING] = 0
                occupied[B_KINGSIDE_ROOK] = 0
                b_occupied[B_KING] = 0
                b_occupied[B_KINGSIDE_ROOK] = 0
                occupied[B_KING + 1] = 1
                occupied[B_KING + 2] = 1
                b_occupied[B_KING + 1] = 1
                b_occupied[B_KING + 2] = 1
                del occupying_pieces[B_KING]
                del occupying_pieces[B_KINGSIDE_ROOK]
                occupying_pieces[B_KING + 1] = 4
                occupying_pieces[B_KING + 2] = 0
                position[1][0][0] += 2
                position[1][4].remove(B_KINGSIDE_ROOK)
                position[1][4].append(B_KING + 1)
            else:
                occupied[W_KING] = 0
                occupied[W_KINGSIDE_ROOK] = 0
                w_occupied[W_KING] = 0
                w_occupied[W_KINGSIDE_ROOK] = 0
                occupied[W_KING + 1] = 1
                occupied[W_KING + 2] = 1
                w_occupied[W_KING + 1] = 1
                w_occupied[W_KING + 2] = 1
                del occupying_pieces[W_KING]
                del occupying_pieces[W_KINGSIDE_ROOK]
                occupying_pieces[W_KING + 1] = 4
                occupying_pieces[W_KING + 2] = 0
                position[0][0][0] += 2
                position[0][4].remove(W_KINGSIDE_ROOK)
                position[0][4].append(W_KING + 1)
        else:   #Queenside
            if colour:
                occupied[B_KING] = 0
                occupied[B_QUEENSIDE_ROOK] = 0
                b_occupied[B_KING] = 0
                b_occupied[B_QUEENSIDE_ROOK] = 0
                occupied[B_KING - 1] = 1
                occupied[B_KING - 2] = 1
                b_occupied[B_KING - 1] = 1
                b_occupied[B_KING - 2] = 1
                del occupying_pieces[B_KING]
                del occupying_pieces[B_QUEENSIDE_ROOK]
                occupying_pieces[B_KING - 1] = 4
                occupying_pieces[B_KING - 2] = 0
                position[1][0][0] -= 2
                position[1][4].remove(B_QUEENSIDE_ROOK)
                position[1][4].append(B_KING - 1)
            else:
                occupied[W_KING] = 0
                occupied[W_QUEENSIDE_ROOK] = 0
                w_occupied[W_KING] = 0
                w_occupied[W_QUEENSIDE_ROOK] = 0
                occupied[W_KING - 1] = 1
                occupied[W_KING - 2] = 1
                w_occupied[W_KING - 1] = 1
                w_occupied[W_KING - 2] = 1
                del occupying_pieces[W_KING]
                del occupying_pieces[W_QUEENSIDE_ROOK]
                occupying_pieces[W_KING - 1] = 4
                occupying_pieces[W_KING - 2] = 0
                position[0][0][0] -= 2
                position[0][4].remove(W_QUEENSIDE_ROOK)
                position[0][4].append(W_KING - 1)
        return

    elif flags == 4:    #Standard capture:
        occupied[from_square] = 0
        if colour:
            b_occupied[from_square] = 0
            b_occupied[to_square] = 1
            w_occupied[to_square] = 0
        else:
            w_occupied[from_square] = 0
            w_occupied[to_square] = 1
            b_occupied[to_square] = 0   
        from_piece = occupying_pieces[from_square]
        to_piece = occupying_pieces[to_square]
        del occupying_pieces[from_square]
        occupying_pieces[to_square] = from_piece
        position[colour][from_piece].remove(from_square)
        position[1^colour][to_piece].remove(to_square)
        position[colour][from_piece].append(to_square)
        captures[num_moves] = to_piece
        return

    elif flags == 5:    #En-Passent:
        if colour:
            occupied[from_square] = 0
            b_occupied[from_square] = 0
            occupied[to_square] = 1
            b_occupied[to_square] = 1
            occupied[to_square + 10] = 0
            w_occupied[to_square + 10] = 0
            del occupying_pieces[from_square]
            del occupying_pieces[to_square + 10]
            occupying_pieces[to_square] = 1
            position[1][1].remove(from_square)
            position[1][1].append(to_square)
            position[0][1].remove(to_square + 10)
            captures[num_moves] = 1
        else:
            occupied[from_square] = 0
            w_occupied[from_square] = 0
            occupied[to_square] = 1
            w_occupied[to_square] = 1
            occupied[to_square - 10] = 0
            b_occupied[to_square - 10] = 0
            del occupying_pieces[from_square]
            del occupying_pieces[to_square - 10]
            occupying_pieces[to_square] = 1
            position[0][1].remove(from_square)
            position[0][1].append(to_square)
            position[1][1].remove(to_square - 10)
            captures[num_moves] = 1
        return

    else:   #Promotion
        promoting_piece = 2 + (flags & 3)
        if flags & 4:   #Capture
            if colour:
                occupied[from_square]=0
                w_occupied[to_square]=0
                b_occupied[from_square] = 0
                b_occupied[to_square] = 1
                to_piece = occupying_pieces[to_square]
                captures[num_moves] = to_piece
                del occupying_pieces[from_square]
                occupying_pieces[to_square] = promoting_piece
                position[0][to_piece].remove(to_square)
                position[1][1].remove(from_square)
                position[1][promoting_piece].append(to_square)
            else:
                occupied[from_square]=0
                b_occupied[to_square]=0
                w_occupied[from_square] = 0
                w_occupied[to_square] = 1
                to_piece = occupying_pieces[to_square]
                captures[num_moves] = to_piece
                del occupying_pieces[from_square]
                occupying_pieces[to_square] = promoting_piece
                position[1][to_piece].remove(to_square)
                position[0][1].remove(from_square)
                position[0][promoting_piece].append(to_square)
        else:       #Quiet promotion
            if colour:
                occupied[from_square]=0
                occupied[to_square]=1
                b_occupied[from_square] = 0
                b_occupied[to_square] = 1
                del occupying_pieces[from_square]
                occupying_pieces[to_square] = promoting_piece
                position[1][1].remove(from_square)
                position[1][promoting_piece].append(to_square)
            else:
                occupied[from_square]=0
                occupied[to_square]=1
                w_occupied[from_square] = 0
                w_occupied[to_square] = 1
                del occupying_pieces[from_square]
                occupying_pieces[to_square] = promoting_piece
                position[0][1].remove(from_square)
                position[0][promoting_piece].append(to_square)
        return

def unmake_move():
    """Unmakes the previous move given the history stored in the global
    variables"""
    global position, moves, num_moves, captures, irreversible, w_occupied, b_occupied, occupied, occupying_pieces
    if num_moves == 0:
        return
    else:
        #Restore irreversible factors
        #Arrays for the last move and irreversibles contain temporary garbage
        colour = 1^position[2][0]   #Colour who made the last move
        last_move = moves[num_moves - 1]
        flags, from_square, to_square = move_info(last_move)
        irreversibles = irreversible[num_moves - 1]
        position[2][0] ^= 1
        if colour:
            position[2][5] -= 1
        position[2][1], position[2][2], position[2][3], position[2][4] = irreversibles
        if flags in [0,1]:  #quiet move
            #Update occupation sets
            occupied[from_square] = 1
            occupied[to_square] = 0
            if colour:
                b_occupied[from_square] = 1
                b_occupied[to_square] = 0
            else:
                w_occupied[from_square] = 1
                w_occupied[to_square] = 0
            piece_code = occupying_pieces[to_square]
            del occupying_pieces[to_square]
            occupying_pieces[from_square] = piece_code
            position[colour][piece_code].remove(to_square)
            position[colour][piece_code].append(from_square)
            num_moves -= 1
            return

        if flags in [2,3]:  #Castles
            if flags == 2:  #Kingside
                if colour:
                    occupied[B_KING] = 1
                    occupied[B_KINGSIDE_ROOK] = 1
                    b_occupied[B_KING] = 1
                    b_occupied[B_KINGSIDE_ROOK] = 1
                    occupied[B_KING + 1] = 0
                    occupied[B_KING + 2] = 0
                    b_occupied[B_KING + 1] = 0
                    b_occupied[B_KING + 2] = 0
                    del occupying_pieces[B_KING + 1]
                    del occupying_pieces[B_KING + 2]
                    occupying_pieces[B_KINGSIDE_ROOK] = 4
                    occupying_pieces[B_KING] = 0
                    position[1][0][0] -= 2
                    position[1][4].remove(B_KING + 1)
                    position[1][4].append(B_KINGSIDE_ROOK)
                else:
                    occupied[W_KING] = 1
                    occupied[W_KINGSIDE_ROOK] = 1
                    w_occupied[W_KING] = 1
                    w_occupied[W_KINGSIDE_ROOK] = 1
                    occupied[W_KING + 1] = 0
                    occupied[W_KING + 2] = 0
                    w_occupied[W_KING + 1] = 0
                    w_occupied[W_KING + 2] = 0
                    del occupying_pieces[W_KING + 1]
                    del occupying_pieces[W_KING + 2]
                    occupying_pieces[W_KING] = 0 
                    occupying_pieces[W_KINGSIDE_ROOK] = 4
                    position[0][0][0] -= 2
                    position[0][4].remove(W_KING + 1)
                    position[0][4].append(W_KINGSIDE_ROOK)
            else:           #Queenside
                if colour:
                    occupied[B_KING] = 1
                    occupied[B_QUEENSIDE_ROOK] = 1
                    b_occupied[B_KING] = 1
                    b_occupied[B_QUEENSIDE_ROOK] = 1
                    occupied[B_KING - 1] = 0
                    occupied[B_KING - 2] = 0
                    b_occupied[B_KING - 1] = 0
                    b_occupied[B_KING - 2] = 0
                    del occupying_pieces[B_KING - 1]
                    del occupying_pieces[B_KING - 2]
                    occupying_pieces[B_KING] = 0
                    occupying_pieces[B_QUEENSIDE_ROOK] = 4
                    position[1][0][0] += 2
                    position[1][4].remove(B_KING - 1)
                    position[1][4].append(B_QUEENSIDE_ROOK)
                else:
                    occupied[W_KING] = 1
                    occupied[W_QUEENSIDE_ROOK] = 1
                    w_occupied[W_KING] = 1
                    w_occupied[W_QUEENSIDE_ROOK] = 1
                    occupied[W_KING - 1] = 0
                    occupied[W_KING - 2] = 0
                    w_occupied[W_KING - 1] = 0
                    w_occupied[W_KING - 2] = 0
                    del occupying_pieces[W_KING - 1]
                    del occupying_pieces[W_KING - 2]
                    occupying_pieces[W_KING] = 0
                    occupying_pieces[W_QUEENSIDE_ROOK] = 4
                    position[0][0][0] += 2
                    position[0][4].remove(W_KING - 1)
                    position[0][4].append(W_QUEENSIDE_ROOK)
            num_moves -= 1
            return
        elif flags == 4:    #Standard capture:
            occupied[from_square] = 1
            if colour:
                b_occupied[from_square] = 1
                b_occupied[to_square] = 0
                w_occupied[to_square] = 1
            else:
                w_occupied[from_square] = 1
                w_occupied[to_square] = 0
                b_occupied[to_square] = 1
            
            from_piece = occupying_pieces[to_square]
            to_piece = captures[num_moves]
            del captures[num_moves]
            occupying_pieces[from_square] = from_piece
            occupying_pieces[to_square] = to_piece
            position[colour][from_piece].remove(to_square)
            position[1^colour][to_piece].append(to_square)
            position[colour][from_piece].append(from_square)
            num_moves -= 1
            return
        
        elif flags == 5:    #En-Passent:
            if colour:
                occupied[from_square] = 1
                b_occupied[from_square] = 1
                occupied[to_square] = 0
                b_occupied[to_square] = 0
                occupied[to_square + 10] = 1
                w_occupied[to_square + 10] = 1
                del occupying_pieces[to_square]
                occupying_pieces[to_square + 10] = 1
                occupying_pieces[from_square] = 1
                position[1][1].remove(to_square)
                position[1][1].append(from_square)
                position[0][1].append(to_square + 10)
                del captures[num_moves]
            else:
                occupied[from_square] = 1
                w_occupied[from_square] = 1
                occupied[to_square] = 0
                w_occupied[to_square] = 0
                occupied[to_square - 10] = 1
                b_occupied[to_square - 10] = 1
                del occupying_pieces[to_square]
                occupying_pieces[to_square - 10] = 1
                occupying_pieces[from_square] = 1
                position[0][1].remove(to_square)
                position[0][1].append(from_square)
                position[1][1].append(to_square - 10)
                del captures[num_moves]
            num_moves -= 1
            return
        else:   #Promotion
            promoting_piece = 2 + (flags & 3)
            if flags & 4:   #Capture
                if colour:
                    occupied[from_square]=1
                    w_occupied[to_square]=1
                    b_occupied[from_square] = 1
                    b_occupied[to_square] = 0
                    to_piece = captures[num_moves]
                    del captures[num_moves]
                    occupying_pieces[to_square] = to_piece
                    occupying_pieces[from_square] = 1
                    position[0][to_piece].append(to_square)
                    position[1][1].append(from_square)
                    position[1][promoting_piece].remove(to_square)
                else:
                    occupied[from_square]=1
                    b_occupied[to_square]=1
                    w_occupied[from_square] = 1
                    w_occupied[to_square] = 0
                    to_piece = captures[num_moves]
                    del captures[num_moves]
                    occupying_pieces[to_square] = to_piece
                    occupying_pieces[from_square] = 1
                    position[1][to_piece].append(to_square)
                    position[0][1].append(from_square)
                    position[0][promoting_piece].remove(to_square)
            else:       #Quiet promotion
                if colour:
                    occupied[from_square]=1
                    occupied[to_square]=0
                    b_occupied[from_square] = 1
                    b_occupied[to_square] = 0
                    del occupying_pieces[to_square]
                    occupying_pieces[from_square] = 1
                    position[1][1].append(from_square)
                    position[1][promoting_piece].remove(to_square)
                else:
                    occupied[from_square]=1
                    occupied[to_square]=0
                    w_occupied[from_square] = 1
                    w_occupied[to_square] = 0
                    del occupying_pieces[to_square]
                    occupying_pieces[from_square] = 1
                    position[0][1].append(from_square)
                    position[0][promoting_piece].remove(to_square)
            num_moves -= 1
            return

def generate_moves():
    """Generates the moves using the global data structures"""
    if position[2][0]:
        return legal_moves_improved(position, b_occupied, w_occupied, occupied,
                                 occupying_pieces)
    else:
        return legal_moves_improved(position, w_occupied, b_occupied, occupied,
                                 occupying_pieces)
        
best_move_found = 0  #Initialization of best move
def search(depth):
    """Main search routine, using negamax algorithm"""
    global best_move_found
    if depth == 0:
        return evaluate(position)
    best_score = -10000
    best_move = 0
    colour = position[2][0]
    if not position[colour][0]:
        return -10000
    elif not position[1^colour][0]:
        return 10000
    move_list, in_check = generate_moves()
    if not len(move_list):
        return (-10000 if in_check else 0)
    for move in move_list:
        make_move(move)
        score = -search(depth - 1)
        unmake_move()
        if score >= best_score:
            best_score = score
            best_move = move                
    best_move_found = best_move
    return best_score
    
def encode_inputted_move(move):
    """Given a user input, returns the move encoding"""
    move_list ,check = generate_moves()
    if move == '0-0':
        from_square = B_KING if position[2][0] else W_KING
        to_square = from_square + 2
        encoded = (2 << 14) | (from_square << 7) | to_square
        if encoded in move_list:
            return encoded
        else:
            return 0
    elif move == '0-0-0':
        from_square = B_KING if position[2][0] else W_KING
        to_square = from_square - 2
        encoded = (3 << 14) | (from_square << 7) | to_square
        if encoded in move_list:
            return encoded
        else:
            return 0

    else:
        from_square = square_to_index_12x10(move[:2])
        to_square = square_to_index_12x10(move[2:4])
        flags = 0
        if occupied[to_square]:
            flags |= 4
        if occupying_pieces[from_square] == 1:
            if abs(from_square - to_square) == 20:
                flags = 1
                encoded = (flags << 14) |  (from_square << 7) | to_square
                if encoded in move_list:
                    return encoded
                else:
                    return 0
                
            if to_square == position[2][3]:
                flags = 5
                encoded = (flags << 14) |  (from_square << 7) | to_square
                if encoded in move_list:
                    return encoded
                else:
                    return 0

            if (position[2][0] and to_square < 30) or ((not position[2][0]) and to_square > 90):
                flags |= 8
                promoting_piece = move[4]
                promoting_flag = ['n','b','r','q'].index(promoting_piece)
                flags += promoting_flag
                encoded = (flags << 14) |  (from_square << 7) | to_square
                if encoded in move_list:
                    return encoded
                else:
                    return 0
        
        encoded = (flags << 14) |  (from_square << 7) | to_square
        if encoded in move_list:
            return encoded
        else:
            return 0
        
def driver():
    global position, moves, num_moves, captures, irreversible, w_occupied, b_occupied, occupied, occupying_pieces
    colour_response = input("White or black? [w/b]")
    colour = 0 if colour_response == 'w' else 1
    load_response = input("Start from a given position? [y/n]")
    load = 1 if load_response == 'y' else 0
    if load:
        fen_input = input("Enter FEN:")
        reinitialize(fen_input)
    print_piece_placements_12x10(position)
    if position[2][0] == colour:
        while True:
            move = encode_inputted_move(input("Enter a move: "))
            if move:
                make_move(move)
                print_piece_placements_12x10(position)
                reset_variables()
                break
            else:
                print("Illegal move")

    while True:
        pos_eval = search(4)
        move_decode_12x10(best_move_found)
        make_move(best_move_found)
        print_piece_placements_12x10(position)
        if pos_eval in [-10000, 10000]:
            print("Checkmate!")
            break
        reset_variables()
        while True:
            move = encode_inputted_move(input("Enter a move: "))
            if move:
                make_move(move)
                print_piece_placements_12x10(position)
                reset_variables()
                break
            else:
                print("Illegal move")
    

def main():
    reinitialize('r1b1k1nr/ppp2p1p/2n3p1/4p3/P3P3/2bP3N/2PB1PPP/R3KB1R b KQkq - 1 12')
    #make_move(encode_inputted_move('c3a1'))
    search(4)
    #move_decode_12x10(best_move_found)

def main2():
    global position, moves, num_moves, captures, irreversible, w_occupied, b_occupied, occupied, occupying_pieces
    position = [[[25], [38, 55, 44, 51, 36, 37, 53], [48], [26, 34], [28], []], [[84], [65, 83, 88, 77, 86, 81, 82], [97, 73], [93, 21], [98, 91], []], [0, 0, 0, 0, 1, 14]]
    w_occupied = colour_occupied_12x10(position, 0)
    b_occupied = colour_occupied_12x10(position, 1)
    occupied, occupying_pieces = all_occupied_12x10(position)
    print_piece_placements_12x10(position)
    #make_move(encode_inputted_move('c3a1'))
    for move in generate_moves()[0]:
        move_decode_12x10(move)
    #print(position[2][1])
    #unmake_move()
    #print(position[2][1])
    #print(search(4))
    #move_decode_12x10(best_move_found)
        
if __name__ == "__main__":
    driver()
    
    
        
        
    
