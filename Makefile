CC = gcc
CFLAGS = -Wall -g 
MAIN = simplified_lz77
OBJECTS = compression.o bit_stream.o queue.o prefix_tree.o hash.o

.PHONY: clean

all:    $(MAIN)

simplified_lz77: simplified_lz77.o $(OBJECTS)
	$(CC) $(CFLAGS) -o simplified_lz77 simplified_lz77.o $(OBJECTS)

test: test.o $(OBJECTS)
	$(CC) $(CFLAGS) -o test test.o $(OBJECTS)
.c.o:
	$(CC) $(CFLAGS) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)
