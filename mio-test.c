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
	double *values;
	int i;
	MIO *mio;
	unsigned long int size;
	int fields;


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

	size = MIOSize(Rfile);
	if((long int)size < 0) {
		fprintf(stderr,"MIOFileSize failed for file %s\n",
			Rfile);
		exit(1);
	}

	printf("attempting open for %s for read with size %ld\n",
			Rfile,
			size);
	mio = MIOOpen(Rfile,"r",size);
	printf("attempting open for %s complete\n",Rfile);

	printf("attempting text field count\n");
	fields = MIOTextFields(mio);
	printf("file %s has %d text fields\n",Rfile,fields);
	

	printf("attempting close for %s for read\n",
			Rfile);
	MIOClose(mio);

	return(0);
}


	

	
	

