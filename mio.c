#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mio.h"


void MIOClose(MIO *mio)
{
	if(mio->bf != NULL) {
		fclose(mio->bf);
	}
	if(mio->fname != NULL) {
		free(mio->fname);
	}
	if(mio->addr != NULL) {
		msync(mio->addr,mio->size,MS_SYNC);
		munmap(mio->addr,mio->size);
	}
	free(mio);
	return;
}
	
/*
 * same semantics as fopen()
 */
MIO *MIOOpen(char *filename, char *mode, unsigned long int size)
{
	MIO *mio;
	int flen;
	char *f;
	FILE *bf;
	int fd;
	int flags;
	int prot;

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

	mio = (MIO *)Malloc(sizeof(MIO));
	if(mio == NULL) {
		fclose(bf);
		return(NULL);
	}

	flen = strlen(filename);
	f = (char *)Malloc(flen+1);
	if(f == NULL) {
		MIOClose(mio);
		return(NULL);
	}

	strncpy(f,filename,flen);
	mio->fname = f;
	strncpy(mio->mode,mode,2);
	mio->bf = bf;
	mio->fd = fd;

	/*
	 * if it is read only, make the file private, else allow the
	 * backing file to be updated (MAP_SHARED)
	 */
	if(strcmp(mio->mode,"r") == 0) {
		prot = PROT_READ;
		flags = MAP_PRIVATE;
	} else {
		prot = (PROT_READ | PROT_WRITE);
		flags = MAP_PRIVATE;
	}

	mio->addr = mmap(NULL,size,prot,flags,mio->fd,0);
	if(mio->addr == NULL) {
		MIOClose(mio);
		return(NULL);
	}
	mio->size = size;

	return(mio);

}

unsigned long int MIOFileSize(char *file)
{
	int psize;
	unsigned long int size;
	struct stat fs;
	int err;

	err = stat(file,&fs);
	if(err < 0) {
		return(-1);
	}

	psize = getpagesize();

	/*
	 * return a multiple of pages
	 */
	size = fs.st_size / psize;
	size = (size+1) * psize;

	return(size);

}

void *MIOAddr(MIO *mio)
{
	return(mio->addr);
}
