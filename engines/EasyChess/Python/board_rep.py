"""board_rep.py: Contains the basic board representation"""

"""Data structure: Piece lists and "flags", stored using a nested array.
Structure will be as follows:
[[wPieces],[bPieces],[flags]
where
[wPieces] = [[wKing],[wPawns],[wKnights],[wBishops],[wRooks],[wQueens]]
[wPawns] = List of squares which wPawns occupy.
Squares are ordered by an index 0-63, with 0=A1, 1=B1,...,8=A2,9=B2,...,63=H8

[flags] = [active colour, white castling rights, black castling rights,
            en-passent target, halfmove clock, fullmove clock]
active colour = 0 for white, 1 for black
castling rights is a number between 0 and 3. least significant bit is kingside
    rights, other bit is queenside rights
en-passent target is square index for the square behind the pawn pushed 2
    squares. As 0 will never be such a square, the value 0 signifies no square.
Halfmove clock is number of halfmoves since last capture or pawn advance
Fullmove clock is the number of full moves, starting at 0, incremented after
Black move.

Associated will be an array of size 120 which will represent occupied and
empty squares and will be used for move generation.
Imagine this as a 12x10 array, with the first 10 elements of the array
representing [0][i] elements, etc.
--Row 0,1, 10, 11 are off-board
--Columns 0, 9 are off-board too
--Rest represent on-board squares

We'll also have a hashmap which associates each occupied square with the piece
located at that square. We associate the pieces independently of colour,
associating 0 for a king, 1 for a pawn, etc. (indices in the array for the
pieces)


NOTE: There is no functionality to determine threefold repetition. May be
added in future."""

from attack_sets import *
import time

""" Builder and tester functions """
#Indexes for each of the pieces in the FEN representation in the data structure.
FEN_IDXS = {'p':(1,1),'k':(1,0),'q':(1,5),'r':(1,4),'n':(1,2),'b':(1,3),
            'P':(0,1), 'K':(0,0), 'Q':(0,5), 'R':(0,4), 'N':(0,2), 'B':(0,3)}

IDXS_TO_CHARS = ['WK','WP','WN','WB','WR','WQ','bk','bp','bn','bb','br','bq']

FILE_ORDER = ['A','B','C','D','E','F','G','H']

FILE_ORDER_LOWER = ['a','b','c','d','e','f','g','h']

PIECE_IDXS = ['K','P','N','B','R','Q']

def index_to_name_12x10(idx):
    """Converts a number from 0 to 119 (square index) into a string
    representation of the square E.G. 'a2'"""
    board_offset = idx - 21
    file = str(FILE_ORDER_LOWER[board_offset % 10])
    rank = str((board_offset // 10) + 1)
    return file + rank

def fen_to_piece_set_12x10(fen_string):
    """Converts the FEN representation (a string) to a piece set."""
    piece_part = fen_string[:fen_string.index(' ')]
    piece_placements = [[[],[],[],[],[],[]],[[],[],[],[],[],[]]]
    square_tracker = 91
    # FEN is rank 8 through rank 1, so work down ranks
    for character in piece_part:
        if character.isnumeric():
            square_tracker += int(character)
        elif character == '/':
            square_tracker -= 18
        else:
            b_w_index, piece_index = FEN_IDXS[character]
            square_number = square_tracker
            piece_placements[b_w_index][piece_index].append(square_tracker)
            square_tracker += 1
    return piece_placements

def fen_to_flags_12x10(fen_string):
    """Extracts the relevant flags from the FEN representation into an array
    to be used in the board representation"""
    CASTLING_RIGHTS = {"KQkq": (3,3), "KQk": (3,1), "KQq": (3,2), "KQ": (3,0),
                       "Kkq": (1,3), "Kk": (1,1), "Kq": (1,2), "K": (1,0),
                       "Qkq": (2,3), "Qk": (2,1), "Qq": (2,2), "Q": (2,0),
                       "kq": (0,3), "k": (0,1), "q": (0,2), "-": (0,0)}
    #Decodes castling rights via hash: probably fastest way
    fen_parts = fen_string.split()
    next_move = 0 if fen_parts[1] == 'w' else 1
    w_rights, b_rights = CASTLING_RIGHTS[fen_parts[2]]
    if fen_parts[3] != '-':
        file = ord(fen_parts[3][0]) - 97
        #97=ord('a'): Calculates alphabet difference
        rank = int(fen_parts[3][1]) - 1
        ep_target = (rank*10) + file  + 21
    else:
        ep_target = 0
    halfmove_clock = int(fen_parts[4])
    fullmove_clock = int(fen_parts[5])

    return [next_move, w_rights, b_rights, ep_target, halfmove_clock, fullmove_clock]

def fen_to_position_12x10(fen_string):
    """Converts the whole FEN to the data structure representing the position"""
    return fen_to_piece_set_12x10(fen_string) + [fen_to_flags_12x10(fen_string)]

def print_board_array_12x10(board_array):
    """Prints the relevant rows of the 12x10 array"""
    for i in range(8):
        print(board_array[9-i][1:9])

def print_piece_placements_12x10(position):
    """Given a position, prints location of pieces using board print routine"""
    all_squares = [['  ']*10,['  ']*10,['  ']*10,['  ']*10,['  ']*10,['  ']*10,['  ']*10,['  ']*10,['  ']*10,['  ']*10,['  ']*10,['  ']*10]
    for idx in range(12):
        colour = idx // 6
        piece_idx = idx % 6
        piece_code = IDXS_TO_CHARS[idx]
        for square_idx in position[colour][piece_idx]:
            row = square_idx // 10
            col = square_idx % 10
            all_squares[row][col] = piece_code
    print_board_array_12x10(all_squares)

def square_to_index_12x10(square_string):
    """Takes a square in string form E.G. 'A1', and converts to the
    corresponding square number"""
    file = FILE_ORDER_LOWER.index(square_string[0])
    rank = int(square_string[1]) - 1
    return (rank * 10) + file + 21

def index_to_square_12x10(index):
    """Takes a square number E.G. 42 for C2 square and converts into string
    form"""
    offset = index - 21
    rank = offset // 10
    file = offset % 10
    return FILE_ORDER[rank] + str(file + 1)

def print_occupying_pieces(occupying_pieces):
    """Given a hashmap of square and occupying piece codes, prints out the
    relevant information in human readable form"""
    for key in occupying_pieces:
        print(f"{index_to_name_12x10(key)}: {PIECE_IDXS[occupying_pieces[key]]}")
        

""" General purpose functions """

def colour_occupied_12x10(position, colour):
    """Returns an array indexed by square number of
    occupied squares by one colour piece: 1=occupied, 0=unoccupied.
    colour argument is 0 for white, 1 for black."""
    occupied_squares = [0]*120
    for piece_idx in range(6):
        for square_idx in position[colour][piece_idx]:
            occupied_squares[square_idx] = 1
    return occupied_squares

def all_occupied_12x10(position):
    """As above, but for both colours. For purposes of move generation, we
    consider off-board squares as occupied.
    Also returns a hashmap/dictionary associating each occupied square to the
    piece located at that square"""
    occupied_squares = [1] * 20 + [1] + [0]*8 + [1]
    occupied_squares += [1] + [0]*8 + [1]
    occupied_squares += [1] + [0]*8 + [1]
    occupied_squares += [1] + [0]*8 + [1]
    occupied_squares += [1] + [0]*8 + [1]
    occupied_squares += [1] + [0]*8 + [1]
    occupied_squares += [1] + [0]*8 + [1]
    occupied_squares += [1] + [0]*8 + [1] + [1] * 20
    occupying_pieces = {}
    for colour in range(2):
        for piece_idx in range(6):
            for square_idx in position[colour][piece_idx]:
                occupied_squares[square_idx] = 1
                occupying_pieces[square_idx] = piece_idx
    return occupied_squares, occupying_pieces

def main():
    test_fen = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'
    test_piece_set = fen_to_piece_set_12x10(test_fen)
    print(test_piece_set)
    test_position = fen_to_position_12x10(test_fen)
    print(test_position)
        

    
    
if __name__ == "__main__":
    main()
