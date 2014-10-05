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
		flags = MAP_SHARED;
	}

	mio->addr = mmap(NULL,size,prot,flags,mio->fd,0);
	if(mio->addr == NULL) {
		MIOClose(mio);
		return(NULL);
	}
	mio->size = size;

	return(mio);

}

MIO *MIOMalloc(unsigned long int size)
{
	MIO *mio;
	int flags;
	int prot;

	mio = (MIO *)Malloc(sizeof(MIO));
	if(mio == NULL) {
		return(NULL);
	}

	/*
	 * if it is read only, make the file private, else allow the
	 * backing file to be updated (MAP_SHARED)
	 */
	prot = (PROT_READ | PROT_WRITE);
	flags = MAP_ANON;

	mio->addr = mmap(NULL,size,prot,flags,-1,0);
	if(mio->addr == NULL) {
		MIOClose(mio);
		return(NULL);
	}
	mio->size = size;

	return(mio);

}

unsigned long int MIOSize(char *file)
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

	return(fs.st_size);

}

void *MIOAddr(MIO *mio)
{
	return(mio->addr);
}

int MIOTextFields(MIO *mio)
{
	FILE *ffd;
	char *line_buff;
	char *ferr;
	int count;
	int found;
	int i;
	int j;
	char c;
	char *l;
	char *separators = MIOSEPARATORS;

	if(mio->fname == NULL) {
		return(-1);
	}

	ffd = fopen(mio->fname,"r");
	if(ffd == NULL) {
		return(-1);
	}

	line_buff = (char *)Malloc(MIOLINESIZE);
	if(line_buff == NULL) {
		return(-1);
	}

	count = -1;
	while(!feof(ffd)) {
		ferr = fgets(line_buff,MIOLINESIZE,ffd);
		if(ferr == NULL) {
			break;
		}
		if(line_buff[0] == '#') {
			continue;
		}
		if(line_buff[0] == '\n') {
			continue;
		}

		/*
		 * clear out any trailing separators
		 */
		i = strlen(line_buff) - 1;
		while(i >= 0) {
			found = 0;
			for(j=0; j < strlen(separators); j++) {
				if(line_buff[i] == separators[j]) {
					line_buff[i] = 0;
					found = 1;
					break;
				}
			}
			if(found == 0) {
				break;
			}
			i--;
		}

		/*
		 * clear out any leading separators
		 */
		l = line_buff;
		i = 0;
		while(i < strlen(line_buff)) {
			found = 0;
			for(j=0; j < strlen(separators); j++) {
				if(line_buff[i] == separators[j]) {
					l++;
					found = 1;
					break;
				}
			}
			if(found == 0) {
				break;
			}
			i++;
		}

		count = 0;
		for(i=0; i < strlen(l); i++) {
			for(j = 0; j < strlen(separators); j++) {
				if(l[i] == separators[j]) {
					count++;
				}
			}
		}
		free(line_buff);
		return(count+1);
	}

	free(line_buff);
	return(count);
}
		
unsigned long int MIOTextRecords(MIO *mio)
{
	FILE *ffd;
	char *line_buff;
	char *ferr;
	unsigned long int count;

	if(mio->fname == NULL) {
		return(-1);
	}

	ffd = fopen(mio->fname,"r");
	if(ffd == NULL) {
		return(-1);
	}

	line_buff = (char *)Malloc(MIOLINESIZE);
	if(line_buff == NULL) {
		return(-1);
	}

	count = 0;
	while(!feof(ffd)) {
		ferr = fgets(line_buff,MIOLINESIZE,ffd);
		if(ferr == NULL) {
			break;
		}
		if(line_buff[0] == '#') {
			continue;
		}
		if(line_buff[0] == '\n') {
			continue;
		}
		count++;
	}

	free(line_buff);
	return(count);
}

MIO *MIODoubleFromText(MIO *t_mio, char *dfname)
{
	int found;
	unsigned long int fsize;
	MIO *d_mio;
	int fields;
	int psize;
	unsigned long int recs;
	unsigned long int min_size;
	unsigned long int size;
	int i;
	int j;
	int k;
	char *separators = MIOSEPARATORS;
	char *curr;
	char *tbuf;
	double *darray;
	unsigned long int row;
	int col;

	fields = MIOTextFields(t_mio);
	if(fields < 0) {
		return(NULL);
	}

	recs = MIOTextRecords(t_mio);
	if(recs < 0) {
		return(NULL);
	}

	min_size = fields*recs*sizeof(double);

	/*
	 * round up to page size
	 */
	psize = getpagesize();
	size = min_size / psize;
	size = (size+1) * psize;

	if(dfname != NULL) {
		d_mio = MIOOpen(dfname,"r+",size);
	} else {
		d_mio = MIOMalloc(size);
	}

	if(d_mio == NULL) {
		return(NULL);
	}

	tbuf = (char *)MIOAddr(t_mio);

	if(tbuf == NULL) {
		MIOClose(d_mio);
		return(NULL);
	}

	darray = MIOAddr(d_mio);
	if(darray == NULL) {
		MIOClose(d_mio);
		return(NULL);
	}

	curr = tbuf;
	fsize = MIOFileSize(t_mio->fname);
	i = 0;
	row = 0;
	col = 0;
	while(i < recs) {
		while((*curr == 0) && (curr < (tbuf + fsize))) {
			curr++;
		}
		if(curr >= (tbuf+fsize)) {
			break;
		}
		found = 0;
		for(j=0; j < strlen(separators); j++) {
			if(*curr == separators[j]) {
				found = 1;
				curr++;
				break;
			}
		}
		/*
		 * did we reach the end of this record?
		 */
		if(*curr == 0) {
			i++;
			continue;
		}
		if(found == 1) {
			continue;
		}

		for(k=0; k < fields; k++) {
			value = strtod(curr,&next);
			darray[row*fields+col]= value;
			col++;
			curr = next+1;
			if((*curr == 0) || (curr >= (tbuf+fsize))) {
				break;
			}
		}

		if(curr > (tbuf+fsize)) {
			break;
		}
		col = 0;
		row++;
	}

	msync(d_mio->addr,d_mio->size,MS_SYNC);
			
		
	return(d_mio);
}
	

