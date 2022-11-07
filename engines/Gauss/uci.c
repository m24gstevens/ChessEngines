#include "uci.h"
#include "board.h"
#include "search.h"
#include "tt.h"

timeinfo_t time_control;

// Forked much of the time-control code from the CPW Engine

static inline bool time_stop(search_info_t* si) {
    if (si->sdepth <= 1) {return false;}
    if (time_control.flags & FINFINITE) {return false;}
    if (time_control.flags & FDEPTH) {return si->sdepth > time_control.depth;}
    if (time_control.flags & FNODES) {return si->nodes > time_control.nodes;}

    if (get_time_ms() >= time_control.stoptime) {
        int movestogo = MOVESTOGO;
        if (time_control.flags & FMOVESTOGO) {movestogo = time_control.movestogo;}
        if ((movestogo > 5) && (time_control.stoptime - time_control.starttime > 5000) &&
            (get_time_ms() - time_control.starttime) < 2*(time_control.stoptime - time_control.starttime)) {return false;}
        else {return true;}
    }
    return false;
}

bool time_stop_root(search_info_t* si) {
    if (time_control.stop) {return true;}

    if (time_control.flags & FINFINITE) {return false;}
    if (time_control.flags & FDEPTH) {return si->sdepth > time_control.depth;}
    if (time_control.flags & FNODES) {return si->nodes > time_control.nodes;}
    if (time_control.flags & FMOVETIME) {return get_time_ms() >= time_control.stoptime;}

    return ((get_time_ms() - time_control.starttime)*2 > (time_control.stoptime - time_control.starttime));
}

void communicate(search_info_t* si) {
    if (!time_control.stop) {time_control.stop = time_stop(si);}
}   

void time_calc(board_t* board) {
    int movetime, movestogo;

    time_control.starttime = get_time_ms();
    time_control.stop = false;
    if (time_control.flags & (FINFINITE | FDEPTH | FNODES)) {return;}
    if (time_control.flags & FMOVETIME) {
        if (time_control.movetime > TIMEBUF) {
            time_control.stoptime = time_control.starttime + time_control.movetime - TIMEBUF;
        } else {
            time_control.stoptime = time_control.starttime;
        }
        return;
    }
    movetime = 0;
    movestogo = MOVESTOGO;
    if (time_control.flags & FMOVESTOGO) {movestogo = time_control.movestogo + 2;}

    if (time_control.time[board->side] < 0) {time_control.time[board->side] = 0;}
    if (time_control.inc[board->side] < 0) {time_control.inc[board->side] = 0;}

    if (time_control.flags & FTIME) {movetime += time_control.time[board->side] / movestogo;}
    if (time_control.flags & FINC) {movetime += time_control.inc[board->side];}

    if (movetime > TIMEBUF) {
        time_control.stoptime = time_control.starttime + movetime - TIMEBUF;
    } else {
        time_control.stoptime = time_control.starttime;
    }
    return;
}

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
    int hply = 0;
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
            board->game_history[hply++] = board->hash;
            make_move(board,mov,&undo);

            while (*curr && *curr != ' ') curr++;
            curr++;
        }
    }
    board->hply = hply;
}

void parse_go(board_t* board, char* command) {
    char* curr = command;
    time_control.flags = 0;

    if (strstr(command, "infinite")) {
        time_control.flags |= FINFINITE;
    }
    if (strstr(command, "searchmoves")) {
        // Not implemented
        time_control.flags |= FINFINITE;
    }

    if ((curr=strstr(command,"wtime"))!=NULL) {
        time_control.flags |= FTIME;
        time_control.time[WHITE] = atoi(curr + 6);
    }
    if ((curr=strstr(command,"btime"))!=NULL) {
        time_control.flags |= FTIME;
        time_control.time[BLACK] = atoi(curr + 6);
    }
    if ((curr=strstr(command,"winc"))!=NULL) {
        time_control.flags |= FINC;
        time_control.inc[WHITE] = atoi(curr + 5);
    }
    if ((curr=strstr(command,"binc"))!=NULL) {
        time_control.flags |= FINC;
        time_control.inc[BLACK] = atoi(curr + 5);
    }
    if ((curr=strstr(command,"movestogo"))!=NULL) {
        time_control.flags |= FMOVESTOGO;
        time_control.movestogo = atoi(curr + 10);
    }
    if ((curr=strstr(command,"depth"))!=NULL) {
        time_control.flags |= FDEPTH;
        time_control.depth = atoi(curr + 6);
    }
    if ((curr=strstr(command,"nodes"))!=NULL) {
        time_control.flags |= FNODES;
        time_control.nodes = atoi(curr + 6);
    }
    if ((curr=strstr(command,"mate"))!=NULL) {
        // Not implemented: Just do an infinite search
        time_control.flags |= FINFINITE | FMATE;
        time_control.mate = atoi(curr + 5);
    }
    if ((curr=strstr(command,"movetime"))!=NULL) {
        // Not implemented: Just do an infinite search
        time_control.flags |= FMOVETIME;
        time_control.movetime = atoi(curr + 9);
    }

    if (time_control.flags == 0) {time_control.flags |= FINFINITE;}

    search_position(board);
}

void send_move(U16 move) {
    printf("bestmove ");
    print_move(move);
    printf("\n");
}

void parse_option(board_t* board, char* command) {
    char* curr = command;
    long val;

    if ((curr = strstr(command,"hash")) != NULL) {
        val = atoi(curr + 5);
        if (val < 1) {val = 64;}
        if (val > 1024) {val = 1024;}

        tt_setsize(val << 20);
    }
}

void uci_loop(board_t* board) {
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    char input[4096];
    printf("id name Gauss\n");
    printf("id author Matt Stevens\n");

    printf("option name hash type spin default 64 min 1 max 1024\n");

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
        else if (strncmp(input, "setoption",9)==0) {
            parse_option(board, input);
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