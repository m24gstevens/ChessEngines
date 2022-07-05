CC=gcc
CFLAGS = -g
DEPS = KQK.h
OBJ = KQK.o KQKGen.o KQKrepr.o KQKio.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

KQK: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)