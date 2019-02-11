# Makefile for this stupid script
CC=gcc
CFLAGS=-Wall -Werror -Wno-unused -DSQROOGE_H
NAME=cwb

# Does order ever really matter?
go: vendor/single.o
	$(CC) $(CFLAGS) vendor/single.o main.c -L. -lgumbo -o $(NAME) && ./$(NAME)

build:
	$(CC) $(CFLAGS) main.c -L. -lgumbo -o $(NAME) 

echo:
	echo $(CC) $(CFLAGS) main.c -L. -lgumbo -o $(NAME) 

main:
	$(CC) $(CFLAGS) m.c -o mm

clean:
	-rm -f *.o vendor/*.o
