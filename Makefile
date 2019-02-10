# Makefile for this stupid script
CC=gcc
CFLAGS=-Wall -Werror -Wno-unused -DSQROOGE_H


# Does order ever really matter?
go: vendor/single.o
	$(CC) $(CFLAGS) vendor/single.o main.c -L. -lgumbo -o pr && ./pr

build:
	$(CC) $(CFLAGS) main.c -L. -lgumbo -o pr 

echo:
	echo $(CC) $(CFLAGS) main.c -L. -lgumbo -o pr 

main:
	$(CC) $(CFLAGS) m.c -o mm

clean:
	-rm -f *.o vendor/*.o
