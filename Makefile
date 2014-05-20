#!/usr/bin/make -f

all: dellfan

dellfan: dellfan.c
	$(CC) -o $@ $^

clean:
	rm -f dellfan
