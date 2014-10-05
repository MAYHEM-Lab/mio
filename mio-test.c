#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "mio.h"

#define ARGS "r:"
char *Usage = "usage: mio-test -r readfile\n";
char Rfile[4096];

int main(int argc, char **argv)
{
	int c;
	int fields;
	double *values;
	int i;
	MIO *mio;
	unsigned long int size;


	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch (c) {
			case 'r':
				strncpy(Rfile,optarg,sizeof(Rfile));
				break;
			default:
				fprintf(stderr,
			"mio-test unrecognized command: %c\n",
					(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Rfile[0] == 0) {
		fprintf(stderr,"need a file\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	} 

	size = MIOFileSize(Rfile);
	if((long int)size < 0) {
		fprintf(stderr,"MIOFileSize failed for file %s\n",
			Rfile);
		exit(1);
	}

	printf("attempting open for %s for read with size %ld\n",
			Rfile,
			size);
	mio = MIOOpen(Rfile,"r",size);

	printf("attempting close for %s for read\n",
			Rfile);
	MIOClose(mio);

	return(0);
}


	

	
	

