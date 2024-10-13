CC=gcc
CFLAGS=-std=gnu18 -Wall -Wextra -Werror -pedantic
.PHONY: all
all: wsh wsh-dbg
wsh: wsh.c wsh.h
	cp -f $< $@_backup.c
	$(CC) $(CFLAGS) -O2 -o $@ $<

wsh-dbg: wsh.c wsh.h
	$(CC) $(CFLAGS) -Og -ggdb -o $@ $<
.PHONY: clean
clean:
	rm -f wsh