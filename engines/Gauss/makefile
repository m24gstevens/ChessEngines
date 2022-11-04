CC=gcc
CFLAGS = -g
DEPS = gauss.h bitboard.h common.h board.h moves.h
OBJ = gauss.o bitboard.o common.o board.o moves.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

gauss: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o
	rm -f gauss
