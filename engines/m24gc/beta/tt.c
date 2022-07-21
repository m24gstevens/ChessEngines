#include "defs.h"
#include "data.h"
#include "protos.h"

// Clear the hash table
void clear_hash() {
    hash_t *hash_entry;
    for (hash_entry = hash_table; hash_entry < hash_table + hash_entries; hash_entry++) {
        hash_entry->key = 0;
        hash_entry->depth = 0;
        hash_entry->flags = 0;
        hash_entry->score = 0;
    }
}

// Dynamically allocate memory for the hash table
void init_hash_table(int mb) {
    int hash_size = 0x100000 * mb;
    hash_entries = hash_size / sizeof(hash_t);
    if (hash_table != NULL) {
        // Clear the hash memory
        free(hash_table);
    }
    hash_table = (hash_t *) malloc(hash_entries * sizeof(hash_t));

    if (hash_table == NULL) {
        // Failed to allocate memory
        init_hash_table(mb / 2);
    }
    else {
        // Allocation worked
        clear_hash();
    }
}

// Initialize pseudo-random keys used for Zobrist hashing
void init_random_keys() {
    int i,j;
    for (i=K; i<=q; i++) {
        for (j=0; j<64; j++)
            piece_keys[i][j] = random_u64();
    }
    // Empty squares
    for (j=0; j<64; j++)
        piece_keys[12][j] = C64(0);
    for (i=0; i < 64; i++)
        en_passent_keys[i] = random_u64();
    for (i=0; i<16; i++)
        castle_keys[i] = random_u64();
    side_key = random_u64();
}

// Generate hash key from the current position
U64 generate_hash() {
    int i, pc;
    U64 bitboard;
    U64 final_key = C64(0);

    for (pc=K; pc <= q; pc++) {
        bitboard = bitboards[pc];
        while (bitboard) {
            int square = bitscanForward(bitboard);
            clear_ls1b(bitboard);
            final_key ^= piece_keys[pc][square];
        }
    }
    if (en_passent_legal) {
        final_key ^= en_passent_keys[en_passent_square];
    }
    final_key ^= castle_keys[castling_rights];
    if (side_to_move) final_key ^= side_key;

    return final_key;
}

void test_hash(int depth) {
    int i;
    if (hash != generate_hash()) {
        print_board();
       printf("incrementally updated: %llx\n Actual: %llx\n", hash, generate_hash());
    }
    if (depth == 0)
        return;
    generate_moves();
    for (i=moves_start_idx[ply]; i<moves_start_idx[ply+1];i++) {
        make_move(move_stack.moves[i].move);
        test_hash(depth - 1);
        unmake_move();
    }
}

int probe_hash(int depth, U16 *best_move, int alpha, int beta) {
    // Lookup
    hash_t *phash = &hash_table[hash % hash_entries];
    if (phash->key == hash) {
        // Found a match
        if (phash->depth >= depth) {
            if (phash->flags == HASH_FLAG_EXACT)
                return phash->score;
            if ((phash->flags == HASH_FLAG_ALPHA) && (phash->score <= alpha))
                return alpha;
            if ((phash->flags == HASH_FLAG_BETA) && (phash->score >= beta))
                return beta;
        }
        // Store the best move
        *best_move = phash->best_move;
    }
    return NO_HASH_ENTRY;
}

void record_hash(int depth, U16 best_move, int score, int hash_flag) {
    hash_t *phash = &hash_table[hash % hash_entries];
    phash->key = hash;
    phash->score = score;
    phash->flags = hash_flag;
    phash->depth = depth;
    phash->best_move = best_move;
}