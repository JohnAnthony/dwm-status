CFLAGS=--std=c11 -pedantic -Wall -Os
.PHONY: clean

all: status

clean:
	rm status

status: status.c config.h
	cc $< -o $@ ${CFLAGS}
