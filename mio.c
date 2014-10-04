#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mio.h"


void MIOClose(MIO *md)
{
	if(md->bf != NULL) {
		fclose(bf);
	}
	if(md->fname != NULL) {
		free(md->fname);
	}
	free(md);
	return;
}
	
/*
 * same semantics as fopen()
 */
MIO *MIOOpen(char *filename, char *mode)
{
	MIO *md;
	int flen;
	char *f;
	FILE *bf;
	int fd;
	int flags;

	/*
	 * try to open the file first
	 */
	bf = fopen(filename,mode);
	if(bf == NULL) {
		return(NULL);
	}

	fd = fileno(bf);
	if(fd < 0) {
		fclose(bf);
		return(NULL);
	}

	md = (MIO *)Malloc(sizeof(MIO));
	if(md == NULL) {
		fclose(bf);
		return(NULL);
	}

	flen = strlen(filename);
	f = (char *)Malloc(flen+1);
	if(f == NULL) {
		MIOClose(md);
		return(NULL);
	}

	strncpy(f,filename,flen);
	mio->fname = f;
	strncpy(mio->mode,mode,2);
	mio->bf = bf;
	mio->fd = fd;

	if(strcmp(mio->mode,"r") == 0) {
		flags = PROT_READ;
	} else {
		flags = (PROT_READ | PROT_WRITE);
	}

XXX

}

	

