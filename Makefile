# Makefile for this stupid script
CC=gcc
CFLAGS=-Wall -Wno-unused


# Does order ever really matter?
par:
	$(CC) $(CFLAGS) p.c -L. -lgumbo -o pr 

echo:
	echo $(CC) $(CFLAGS) p.c -L. -lgumbo -o pr 

main:
	$(CC) $(CFLAGS) m.c -o mm

