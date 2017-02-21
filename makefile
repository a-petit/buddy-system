CC = gcc
CFLAGS = -std=c11 -Wall -Wconversion -Werror -Wextra -Wpedantic -O2
LDFLAGS = -lm
objects = heap.o main.o
executable = main

all: $(executable)

clean:
	$(RM) $(objects) $(executable)

$(executable): $(objects)
	$(CC) $(objects) $(LDFLAGS) -o $(executable)

heap.o: heap.c heap.h
main.o: main.c heap.h
