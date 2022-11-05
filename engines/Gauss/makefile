CC=gcc
CFLAGS = -g -O3
DEPS = gauss.h bitboard.h common.h board.h moves.h uci.h eval.h search.h
OBJ = gauss.o bitboard.o common.o board.o moves.o uci.o	eval.o search.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

gauss: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o
	rm -f gauss
