CFLAGS=--std=c11 -pedantic -Wall

all: status

clean:
	rm status

status: status.c
	cc $< -o $@ ${CFLAGS}
