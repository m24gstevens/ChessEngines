#include "defs.h"
#include "data.h"
#include "protos.h"

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

void clear_hash() {
    for (int index = 0; index < HASH_SIZE; index++) {
        hash_t entry = hash_table[index];
        entry.key = C64(0);
        entry.depth = 0;
        entry.flags = 0;
        entry.score = 0;
    }
}

int probe_hash(int depth, int alpha, int beta) {
    // Lookup
    hash_t *phash = &hash_table[hash % HASH_SIZE];
    if (phash->key == hash) {
        // Found a match
        if (phash->depth >= depth) {
            int score = phash->score;

            if (score < -MATE_THRESHOLD) score += ply;
            if (score > MATE_THRESHOLD) score -= ply;

            if (phash->flags == HASH_FLAG_EXACT)
                return score;
            if ((phash->flags == HASH_FLAG_ALPHA) && (score <= alpha))
                return alpha;
            if ((phash->flags == HASH_FLAG_BETA) && (score >= beta))
                return beta;
        }
    }
    return NO_HASH_ENTRY;
}

void record_hash(int depth, int score, int hash_flag) {
    hash_t *phash = &hash_table[hash % HASH_SIZE];

    // Adjust for mating scores, as now we will make them independent of a position
    if (score < -MATE_THRESHOLD) score -= ply;
    if (score > MATE_THRESHOLD) score += ply;

    phash->key = hash;
    phash->score = score;
    phash->flags = hash_flag;
    phash->depth = depth;
}