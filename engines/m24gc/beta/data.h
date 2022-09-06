// Header file to include the data
extern U64 pawn_attack_table[2][64]; 
extern U64 knight_attack_table[64];
extern U64 king_attack_table[64];
extern U64 queen_attack_table[64];

extern U64 bishopAttackTable[64][512]; 
extern U64 rookAttackTable[64][4096];

extern U64 bishopPinTable[64][512]; 
extern U64 rookPinTable[64][4096]; 

extern U64 promoting_ranks[2];

extern U64 bitboards[12];
extern U64 occupancies[3];
extern char piece_on_square[64];

extern int side_to_move;
extern int castling_rights;
extern int en_passent_legal;
extern int en_passent_square;

extern int material_count[2];
extern int simple_piece_values[12];
extern int positive_simple_piece_values[12];

extern int ply;
extern int hply;
extern int fifty_move;
extern U64 hash;
extern counter_move last_move;

extern U64 repetition_table[MAX_HIST];
extern int repetition_index;

extern char *piece_characters;
extern char char_to_piece_code[];

extern char *square_strings[64];

extern char *promoted_pieces;

extern _move_list move_stack;
extern int moves_start_idx[MAX_HIST];
extern hist_t game_history[MAX_HIST];

extern int pv_length[MAX_PLY];
extern U16 pv_table[MAX_PLY][MAX_PLY];
extern int follow_pv;

extern U64 nodes;

extern U64 piece_keys[13][64];
extern U64 en_passent_keys[64];
extern U64 castle_keys[16];
extern U64 side_key;

extern hash_t *hash_table;
extern int hash_entries;

extern char *starting_position;
extern char *kiwipete;
extern char *cmk_position;
extern char *headache_position;
extern char *repetitions;
extern char *italian_game;
extern char *sharp_najdorf;

extern int quit;
extern int movestogo;
extern int movetime;
extern int _time;
extern int inc;
extern int starttime;
extern int stoptime;
extern int timeset;
extern int stopped;