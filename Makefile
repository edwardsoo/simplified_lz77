CC = gcc
CFLAGS = -Wall -g 
MAIN = simplified_lz77

.PHONY: clean

all:    $(MAIN)

simplified_lz77: simplified_lz77.o bit_stream.o queue.o
	$(CC) $(CFLAGS) -o simplified_lz77 simplified_lz77.o bit_stream.o queue.o

.c.o:
	$(CC) $(CFLAGS) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)
