#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mio.h"

MIO *MIOCreate(char *filename)
{
	MIO *md;
	int flen;
	char *f;

	md = (MIO *)malloc(sizeof(MIO));
	if(md == NULL) {
		return(NULL);
	}
	memset(md,0,sizeof(MIO));

	flen = strlen(filename);
	f = (char *)malloc(flen+1);
	if(f == NULL) {
		free(md);
		return(NULL);
	}
	memset(f,0,flen);
	memcpy(f,filename,flen);

	mio->fname = f;

XXX
}
	

	

