#ifndef MIO_H
#define MIO_H

struct mio_stc
{
	char *fname;
	char mode[3];	/* mode is 2 char for fopen() */
	FILE *bf;
	void *addr;
};

typedef struct mio_stc MIO;

#endif

