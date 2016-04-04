CFLAGS=--std=c11 -pedantic -Wall
LIBS=-lxcb
.PHONY: clean

all: status

clean:
	rm status

status: status.c config.h
	cc $< -o $@ ${CFLAGS} ${LIBS}
