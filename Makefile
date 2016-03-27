all: status

status: status.c
	cc $< -o $@ -I/usr/src/linux/include ${CFLAGS}
