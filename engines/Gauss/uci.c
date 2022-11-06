#include "uci.h"
#include "board.h"
#include "search.h"

// Parse a move
U16 parse_move(board_t* board, char* s) {
    U8 from, to;
    int i,ct;
    U16 move;
    enumPiece promoted;

    from =  (s[0]-'a') + 8*(s[1]-'1');
    to = ((s[2]-'a') + 8*(s[3]-'1'));
    move_t moves[200];
    ct = generate_moves(board,moves);

    for (i=0;i<ct;i++) {
        move = moves[i].move;
        if ((MOVE_FROM(move)==from) && (MOVE_TO(move)==to)) {
            if (!isalpha(s[4])) {return move;}
            else {
                promoted = char_to_piece_code[tolower(s[4])] - n;
                if (IS_PROMOTION(move) && (MOVE_PROMOTE_TO(move) == promoted)) {return move;}
            }
        }
    }
    return NOMOVE;
}

// Parse a UCI 'position' command
void parse_position(board_t* board, char* command) {
    U16 mov = NOMOVE;
    hist_t undo;
    char* curr = command;
    curr += 9;
    // 'position startpos' or 'position fen ...'
    if (strncmp(curr,"startpos",8)==0) {
        parse_fen(board,starting_position);
    } else {
        curr = strstr(command,"fen");
        if (curr == NULL) {parse_fen(board,starting_position);}
        else {
            curr += 4;
            parse_fen(board,curr);
        }
    }
    // Potentially a 'moves ...'
    curr = strstr(curr, "moves");
    if (curr != NULL) {
        curr += 6;
        while (*curr) {
            if (*curr == '\n') {break;}

            mov = parse_move(board,curr);
            if (mov == NOMOVE) {break;}
            history_table[hply++] = board->hash;
            make_move(board,mov,&undo);

            while (*curr && *curr != ' ') curr++;
            curr++;
        }
    }
}

void parse_go(board_t* board, char* command) {
    char* curr = command;
    int depth;

    curr = strstr(curr,"depth");
    if (curr != NULL) {
        depth = atoi(curr+6);
        if (depth < 0) {depth = 3;}
    } else {
        depth = 3;
    }
    search_position(board,depth);
}

void uci_loop(board_t* board) {
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    char input[4096];
    printf("id name Gauss\n");
    printf("id author Matt Stevens\n");
    printf("uciok\n");

    while (1) {
        memset(input, 0, sizeof(input));
        fflush(stdout);

        if (!fgets(input, 4096, stdin)) {continue;}

        if (input[0] == '\n') {continue;}

        else if (strncmp(input, "isready", 7) == 0) {
            printf("readyok\n");
            continue;
        }
        else if (strncmp(input, "position", 8) == 0) {
            parse_position(board, input);
        }
        else if (strncmp(input, "ucinewgame", 10) == 0) {
            parse_position(board,"position startpos");
        }
        else if (strncmp(input, "go", 2) == 0) {
            parse_go(board,input);
        }
        else if (strncmp(input, "quit", 4) == 0) {break;}

        else if (strncmp(input, "uci", 3) == 0) {
            printf("id name Gauss\n");
            printf("id author Matt Stevens\n");
            printf("uciok\n");
        }
    }
}