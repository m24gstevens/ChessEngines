/* tune.c - For tuning the evaluation parameters */

#include "defs.h"
#include "data.h"
#include "protos.h"

// Playing against itself for testing purposes
static void write_game_results(FILE *outfile, char *position_fens[],char *result) {
    int fen_idx = 0;
    char *fen_string;
    while ((fen_string = position_fens[fen_idx++]) != NULL) {
        fprintf(outfile, "%s,%s\n", fen_string, result);
    }
}

int game_finished(int score, int num_moves) {
    // Too many moves bug
    if (num_moves > 650)
        return 1;
    // mating score
    if ((score > 20000) | (score < -20000))
        return 1;
    // fifty move rule
    if (fifty_move >= 100)
        return 1;
    // threefold repetition
    if (reps() >= 2)
        return 1;
    // insufficient material draws
    if (material_draw())
        return 1;
    // lone kings
    if (!(occupancies[WHITE] & (occupancies[WHITE]-1)) && !(occupancies[BLACK] & (occupancies[BLACK]-1)))
        return 1;
    // stalemate
    generate_moves();
    if (moves_start_idx[ply] == moves_start_idx[ply+1])
        return 1;   // Can't be mated in our context
    return 0;
}

void quick_game(FILE *outfile, char *starting_fen) {
    // Set up board representation
    clear_hash();
    char *position_fens[700];
    for (int i=0; i<700; i++)
        position_fens[i] = NULL;

    int offset;
    char win_draw_loss[4];

    char *current_fen = (char *)malloc(100*sizeof(char));
    // Make a random legal move

    int nrm;
    int random_moves_success;
    while (1) {
        nrm = 0;
        random_moves_success = 1;
        while ((nrm++) < 4) {
            random_moves_success &= make_random_move();
        }
        if (random_moves_success)
            break;
    }

    get_fen(current_fen);

    printf("FEN: %s\n", current_fen);

    char *gamestring = (char *)malloc(5000*sizeof(char));
    char *gp = gamestring;
    offset = sprintf(gp, "position %s moves", starting_fen);
    gp = gamestring + offset; 
    /* 
    current_fen = starting_fen

    while the current position isn't ended:
        load_position(current_fen)
        put_fen(current_fen, position_fens)

        score = search()
        best_move = get_best_move()

        load_position(current_fen)
        make_move(best_move)

        current_fen = position_fen()


    if the score is a mating score:
        game_score = 1 if (side_to_move == black) else 0
    else
        game_score = 0.5

    write_results()
    */
    int score = 0;
    U16 best_move;
    int num_moves = 0;
    quickresult_t result;
    char strmove[6];
    timeset = 1;
    while (!game_finished(score, num_moves)) {
        parse_position(gamestring);
        position_fens[num_moves] = (char *)malloc(100*sizeof(char));
        strncpy(position_fens[num_moves++],current_fen,100);

        stoptime = get_time_ms() + 10;

        quickgame_search(&result);
        score = result.evaluation;
        best_move = result.bestmove;
        move_to_string(strmove, best_move);

        parse_position(gamestring);
        make_move(best_move);

        *gp++ = ' ';
        strncpy(gp,strmove,6);
        gp += strlen(strmove);

        get_fen(current_fen);
    }


    if ((score > 20000) | (score < -20000)) 
        strncpy(win_draw_loss,((score * (2*side_to_move -  1)) < 0) ? "1" : "0",2); // White wins if its black to move and score is high, or white to move and score is low
    else
        strncpy(win_draw_loss,"0.5",4);

    printf("%s\n", win_draw_loss);    
    
    write_game_results(outfile, position_fens, win_draw_loss);
    free(current_fen);
    int fen_idx = 0;
    while ((current_fen = position_fens[fen_idx++]) != NULL)
        free(current_fen);
    free(gamestring);
}

// Parsing arena .abk files
struct ABKentry {
    unsigned char from;
    unsigned char to;
    unsigned char promotion;
    unsigned char priority;
    unsigned int ngames;
    unsigned int nwon;
    unsigned int nlost;
    unsigned int plycount;
    int nextMove;
    int nextSibling;
} *ABKbook;

static inline int abk_size(FILE *ABKFile) {
    int sz;
    fseek(ABKFile, 0L, SEEK_END);
    sz = ftell(ABKFile);
    rewind(ABKFile);
    return sz;
}

U16 parse_book_move(unsigned char move_from, unsigned char move_to, unsigned char promotion) {
    char move[6];
    move[0] = ((move_from)&7) + 'a';
    move[1] = ((move_from)>>3) + '1';
    move[2] = ((move_to)&7) + 'a';
    move[3] = ((move_to)>>3) + '1';
    if (promotion) {
        switch (promotion) {
            case 1:
                move[4] = 'R';
                break;
            case 2:
                move[4] = 'N';
                break;
            case 3:
                move[4] = 'B';
                break;
            default:
                move[4] = 'Q';
                break;
        }
        move[5] = '\0';
    } else
        move[4] = '\0';
    print_move(parse_move(move));
    printf("\n");
    return parse_move(move);
}

void traverse_book(int entry_index, char *current_fen, FILE *outfile) {
    /* 
    load the entry at the given index
    if there is a child node:
        make move at the entry
        traverse_book(child index, new current fen)
    if there is a sibling node:
        traverse_book(sibling index, current_fen)
    */
    if (ABKbook[entry_index].nextMove > 0) {
        parse_fen(current_fen);
        make_move(parse_book_move(ABKbook[entry_index].from, ABKbook[entry_index].to, ABKbook[entry_index].promotion));
        char *fen = (char *)malloc(100*sizeof(char));
        get_fen(fen);
        quick_game(outfile, fen);
        traverse_book(ABKbook[entry_index].nextMove, fen, outfile);
        free(fen);
    }
    if (ABKbook[entry_index].nextSibling >= 0) {
        traverse_book(ABKbook[entry_index].nextSibling, current_fen, outfile);
    }
}


// Full testing routine

void test_games(const char *abkname) {
    FILE *ABKFile = fopen(abkname, "rb");
    if (ABKFile == NULL)
        return;

    int file_size = abk_size(ABKFile);
    ABKbook = malloc(file_size);

    fread(ABKbook, file_size/sizeof(struct ABKentry), sizeof(struct ABKentry), ABKFile);
    fclose(ABKFile);

    FILE *test_data = fopen("testgames.csv","a+");
    if (test_data == NULL)
        goto cleanup;

    char *fen = (char *)malloc(100*sizeof(char));
    strcpy(fen,starting_position);

    traverse_book(900, fen, test_data);

    free(fen);

    fclose(test_data);
    cleanup:
        printf("All done!\n");    
}