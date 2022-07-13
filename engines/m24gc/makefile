CC=gcc
CFLAGS = -g -O3
DEPS = m24gc.h uci.h
OBJ = main.o bitboards.o board.o uci.o eval.o search.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

m24gc: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
