#include "defs.h"
#include "data.h"
#include "protos.h"
#include <ctype.h>
#include <stdlib.h>
#ifdef _WIN64
    #include <windows.h>
#else
    # include <sys/time.h>
#endif


/* ========== ====================================================================
Most of this is forked from BBC by Code Monkey King and VICE by Bluefever Software
================================================================================ */

// Initialize timing variables
void init_times() {
    quit = 0;
    movestogo = 30;
    movetime = -1;
    _time = -1;
    inc = 0;
    starttime = 0;
    stoptime = 0;
    timeset = 0;
    stopped = 0;
}

// Get time in ms
int get_time_ms() {
    #ifdef _WIN64
        return GetTickCount();
    #else
        struct timeval time_value;
        gettimeofday(&time_value, NULL);
        return time_value.tv_sec * 1000 + time_value.tv_usec / 1000;
    #endif
}

int input_waiting() {
    #ifndef _WIN64
        fd_set readfds;
        struct timeval tv;
        FD_ZERO (&readfds);
        FD_SET (fileno(stdin), &readfds);
        tv.tv_sec = 0; tv.tv_usec = 0;
        select(16, &readfds, 0, 0, &tv);
    #else
        static int init = 0, pipe;
        static HANDLE inh;
        DWORD dw;

        if (!init) {
            init = 1;
            inh = GetStdHandle(STD_INPUT_HANDLE);
            pipe = !GetConsoleMode(inh, &dw);
            if (!pipe) {
                SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
                FlushConsoleInputBuffer(inh);
            }
        }

        if (pipe) {
            if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) return 1;
            return dw;
        } else {
            GetNumberOfConsoleInputEvents(inh, &dw);
            return dw <=1 ? 0 : dw;
        }
        #endif
}

// Read GUI/User input
void read_input() {
    int bytes;
    char input[256] = "", *endc;
    if (input_waiting()) {
        stopped = 1;
        do {
            bytes = read(fileno(stdin), input, 256);
        } while (bytes < 0);

        endc = strchr(input, '\n');
        if (endc) *endc = 0;
        if (strlen(input) > 0) {
            if (!strncmp(input, "quit",4))
                quit = 1;
            else if (!strncmp(input, "stop", 4))
                quit = 1;
        }
    }
}

// interact between search and GUI
void communicate() {
    if (timeset==1 && get_time_ms() > stoptime) {
        stopped = 1;
    }
    read_input();
}
/* Parses a move and returns the move's U16 if exists, _FALSE if not */
U16 parse_move(char *move_string) {
    int start_count = moves_start_idx[ply];
    int start_square = 8*(move_string[1] - '1') + move_string[0] - 'a';
    int end_square = 8*(move_string[3] - '1') + move_string[2] - 'a';
    generate_moves();
    for (int i=0; i<move_stack.count;i++) {
        U16 move = move_stack.moves[i].move;
        if ((move_source(move) == start_square) && (move_target(move) == end_square)) {
            /* Might make the move */
            if (!isalpha(move_string[4])) {
                return move;
            } else {
                int promoted_piece = char_to_piece_code[tolower(move_string[4])] - n;
                if ((move_flags(move) & 0x8) && ((int)((move_flags(move)) & 0x3) == promoted_piece)) {
                    return move;
                }
            }
        }
    }
    /* Able to make the move */
    return _FALSE;
}

/* Parses a position given a "position" UCI command */
void parse_position(char *command) {
    /* Jump over "position " */
    char *current_command = command;
    command += 9;
    if (strncmp(command,"startpos",8) == 0) {
        /* startpos command */
        parse_fen(starting_position);
    }
    else {
        /* Find a "fen" subcommand */
        current_command = strstr(command,"fen");
        if (current_command == NULL)
            parse_fen(starting_position);
        else {
            /* Found a "fen" instance */
            current_command += 4;
            parse_fen(current_command);
        }
    }
    /* Find the 'moves subcommand */
    current_command = strstr(command,"moves");
    if (current_command != NULL) {
        /* Found some moves */
        current_command += 6;
         while (*current_command) {
            if (*current_command == '\n')
                break;
            U16 move = parse_move(current_command);
            prepare_search();
            if (move) {
                make_move(move);
                /* Find the next token */
                while (*current_command && *current_command != ' ') current_command++;
                current_command++;
            } else
                break;
         }
    }
    return;
}

// Parse the 'go' command
void parse_go(char *command) {
    int depth = -1;
    char *argument = NULL;

    //infinite search
    if ((argument = strstr(command, "infinite"))) {}
    // increment
    if ((argument = strstr(command, "binc")) && side_to_move == BLACK)
        inc = atoi(argument + 5);
    if ((argument = strstr(command, "winc")) && side_to_move == WHITE)
        inc = atoi(argument + 5);
    // time command
    if ((argument = strstr(command, "wtime")) && side_to_move == WHITE) {
        _time = atoi(argument + 6);
    }
    if ((argument = strstr(command, "btime")) && side_to_move == BLACK)
        _time = atoi(argument + 6);
    // movestogo command
    if ((argument = strstr(command, "movestogo")))
        movestogo = atoi(argument + 10);
    // movetime command
    if ((argument = strstr(command, "movetime")))
        movetime = atoi(argument + 9);
    // depth command
    if ((argument = strstr(command, "depth")))
        depth = atoi(argument + 6);
    // if movetime isn't available
    if (movetime != -1) {
        _time = movetime;
        movestogo = 1;
    }
    starttime = get_time_ms();
    depth = depth;
    // time control available
    if (_time != -1) {
        timeset = 1;
        _time /= movestogo;
        _time -= 50;
        stoptime = starttime + _time + inc;
    }
    // no depth available
    if (depth == -1)
        depth = 64;
    printf("time:%d start:%d stop:%d depth %d timeset %d\n", _time, starttime, stoptime, depth, timeset);
    //search
    search(depth);
}

void uci() {
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    char input[2000];
    printf("id name m24gc\n");
    printf("id name Matt Stevens\n");
    printf("uciok\n");

    // main loop
    while (1) {
        memset(input, 0, sizeof(input));
        fflush(stdout);

        if (!fgets(input, 2000, stdin))
            continue;
        if (input[0] == '\n')
            continue;
        // "isready" command
        else if (strncmp(input, "isready", 7) == 0) {
            printf("readyok\n");
            continue;
        // "position" command
        }
        else if (strncmp(input, "position", 8) == 0) {
            parse_position(input);
        }
        
        // "ucinewgame" command
        else if (strncmp(input, "ucinewgame", 10) == 0) {
            parse_position("position startpos");
            //clear hash table
            clear_hash();
        }
        
        // "ucigo" command
        else if (strncmp(input, "go", 2) == 0)
            parse_go(input);

        // "quit" command
        else if (strncmp(input, "quit", 4) == 0)
            break;

        // "uci" command
        else if (strncmp(input, "uci", 3) == 0) {
            printf("id name m24gc\n");
            printf("id name Matt Stevens\n");
            printf("uciok\n");
        }
    }
}
