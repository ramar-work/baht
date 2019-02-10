# Makefile for this stupid script
CC=gcc
CFLAGS=-Wall -Wno-unused


# Does order ever really matter?
go:
	$(CC) $(CFLAGS) p.c -L. -lgumbo -o pr && ./pr

build:
	$(CC) $(CFLAGS) p.c -L. -lgumbo -o pr 

echo:
	echo $(CC) $(CFLAGS) p.c -L. -lgumbo -o pr 

main:
	$(CC) $(CFLAGS) m.c -o mm

