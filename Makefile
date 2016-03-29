CFLAGS=--std=c11 -pedantic -Wall -O2

all: status

clean:
	rm status

status: status.c config.h
	cc $< -o $@ ${CFLAGS}
