# Makefile for this stupid script
CC=gcc
VERSION=0.02
CFLAGS=-Wall -Werror -Wno-unused -DSQROOGE_H -DLT_DEVICE=1 -DVERSION="\"$(VERSION)\"" #-DERRV_H
CC=clang
CFLAGS=-Wall -Werror -Wno-unused -DSQROOGE_H -DLT_DEVICE=1 -DVERSION="\"$(VERSION)\"" -DSEE_FRAMING #-DERRV_H
NAME=baht
DBSERVER="localhost"
DATABASE=ctrial_db
DBUSER=
DBPASSWORD=
SQLBIN=sqlcmd
LDDIRS=lib
LDFLAGS=-lgumbo -lgnutls -llua #-lgnutls -lcurl
PREFIX=/usr/local

# build - Build the 'baht' scraper tool
build: vendor/single.o
	$(CC) $(CFLAGS) -DDEBUG vendor/single.o baht.c -L$(LDDIRS) $(LDFLAGS) -o $(NAME)

# href - Build baht's http handling code 
href: vendor/single.o
	$(CC) $(CFLAGS) -DIS_TEST vendor/single.o web.c -L$(LDDIRS) -lgnutls -lcurl -o ref && ./ref

# filter - Build and test baht filters (under construction, do not use)
filter:
	printf 'This target is currently under construction.  Do not use it.' > /dev/stderr

# explain - List all the targets and what they do
explain:
	@printf 'Available options are:\n'
	@sed -n '/^#/ { s/# //; 1d; p; }' Makefile | awk -F '-' '{ printf "  %-20s - %s\n", $$1, $$2 }'

# clean - clean up
clean:
	-rm -f *.o vendor/*.o
	-rm -f $(NAME)

# install - Install the tool somewhere
install:
	test -d $(PREFIX)/bin/ || mkdir -p $(PREFIX)/bin/
	cp -v $(NAME) $(PREFIX)/bin/	


# uninstall - Install the tool somewhere
uninstall:
	printf ''


# package
pkg:
	git archive --format tar.gz --prefix baht-$(VERSION)/ HEAD > /tmp/baht-$(VERSION).tar.gz


