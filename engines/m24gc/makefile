CC=gcc
CFLAGS = -g -O3
DEPS = data.h defs.h protos.h
OBJ = main.o bitboards.o board.o uci.o eval.o search.o data.o tt.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

m24gc: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
