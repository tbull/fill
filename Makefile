CS = fill.c
OBJS = fill.o
WOBJS = ${OBJS}

CWFLAGS=-Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wnested-externs -Winline
CDFLAGS=-O -g ${CWFLAGS} -DDEBUG -D_DEBUG
CFLAGS=-O2 ${CWFLAGS} -DNDEBUG
CC=gcc


fill: ${OBJS}
	${CC} ${CFLAGS} -lm -o $@ ${OBJS}
	strip $@

debug: ${CS}
	${CC} ${CDFLAGS} -lm -o fill ${CS}

cygwin: ${WOBJS}
	${CC} ${CFLAGS} -lm -o fill.exe ${WOBJS}
	strip fill.exe


${OBJS}: fill.c


.PHONY: clean

clean:
	-rm fill fill.exe ${WOBJS} *.bak

