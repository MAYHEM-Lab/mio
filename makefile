CC=gcc
INC=mymalloc.h
CFLAGS=-g 

LIBS=mymalloc.o

all: mio-test mio-leak-test mio-write-test mio-record-test mio-malloc-test pmalloc-stress

mio.o: mio.c ${INC} mio.h
	${CC} ${CFLAGS} -c mio.c

mymalloc.o: mymalloc.c mymalloc.h
	${CC} ${CFLAGS} -c mymalloc.c

my_malloc.o: my_malloc.c my_malloc.h
	${CC} ${CFLAGS} -c my_malloc.c

pmalloc.o: pmalloc.c pmalloc.h
	${CC} ${CFLAGS} -c pmalloc.c

mio-test: mio-test.c ${INC} mio.h mio.o ${LIBS}
	${CC} ${CFLAGS} -o mio-test mio-test.c mio.o ${LIBS}

mio-write-test: mio-write-test.c ${INC} mio.h mio.o ${LIBS}
	${CC} ${CFLAGS} -o mio-write-test mio-write-test.c mio.o ${LIBS}

mio-leak-test: mio-leak-test.c ${INC} mio.h mio.o ${LIBS}
	${CC} ${CFLAGS} -o mio-leak-test mio-leak-test.c mio.o ${LIBS}

mio-record-test: mio-record-test.c ${INC} mio.h mio.o ${LIBS}
	${CC} ${CFLAGS} -o mio-record-test mio-record-test.c mio.o ${LIBS}

mio-malloc-test: mio-malloc-test.c ${INC} mio.h mio.o my_malloc.o my_malloc.h ${LIBS}
	${CC} ${CFLAGS} -o mio-malloc-test mio-malloc-test.c mio.o my_malloc.o ${LIBS}

pmalloc-stress: pmalloc.o pmalloc.h mio.h mio.o ${LIBS}
	${CC} ${CFLAGS} -o pmalloc-stress pmalloc-stress.c mio.o pmalloc.o ${LIBS}

clean:
	rm *.o mio-test
