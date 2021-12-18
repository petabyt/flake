CC=tcc
CFLAGS=-w

all:
	$(CC) $(CFLAGS) main.c -o ~/.local/bin/flake
	echo "-- Testing flake cli:"
	flake test best

test:
	echo Hello

best:
	echo World
