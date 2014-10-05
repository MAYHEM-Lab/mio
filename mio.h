#ifndef MIO_H
#define MIO_H

#include "mymalloc.h"

struct mio_stc
{
	char *fname;
	char mode[3];	/* mode is 2 char for fopen() */
	FILE *bf;
	int fd;
	void *addr;
	unsigned long int size;
};

typedef struct mio_stc MIO;

MIO *MIOOpen(char *filename, char *mode, unsigned long int size);
void MIOClose(MIO *mio);
unsigned long int MIOFileSize(char *file);

#endif

