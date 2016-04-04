CFLAGS=--std=c11 -pedantic -Wall -O2
LIBS=-lxcb -lxcb-util
.PHONY: clean

all: status

clean:
	rm status

status: status.c config.h
	cc $< -o $@ ${CFLAGS} ${LIBS}
