CC=gcc
INC=mymalloc.h
CFLAGS=-g ${INC}

LIBS=mymalloc.o

mio.o: mio.c ${INC} mio.h
	${CC} ${CFLAGS} -c mio.c

mymalloc.o: mymalloc.c mymalloc.h
	${CC} ${CFLAGS} -c mymalloc.c
