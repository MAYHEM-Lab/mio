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

#define MIOLINESIZE (1024*1024) /* max size of input line */
#define MIOSEPARATORS " \n"	/* separator chars for text parsing */

MIO *MIOOpen(char *filename, char *mode, unsigned long int size);
void MIOClose(MIO *mio);
unsigned long int MIOSize(char *file);
int MIOTextFields(MIO *mio);

#endif

