CC = gcc
CFLAGS = -Wall -g 
MAIN = simplified_lz77

.PHONY: clean

all:    $(MAIN)

simplified_lz77: simplified_lz77.o
	$(CC) $(CFLAGS) -o simplified_lz77 simplified_lz77.o

.c.o:
	$(CC) $(CFLAGS) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)
