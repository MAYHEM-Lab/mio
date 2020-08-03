#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pmalloc.h"

#ifdef HAVE_DRAND48
#define RAND() (drand48())
#define SEED(x) (srand48((x)))
#else
/* not my fault it's not there... */
#define RAND() ((double)random()/RAND_MAX)
#define SEED(x) (srandom((x)))
#endif

#define BUFS (1000)

#define ARGS "f:s:S"

char *Usage = "pmalloc-stress -f filename -s size <-S use sync mode>";

char Fname[4096];
int BufferSize;
int SyncMode;


int main(int argc, char *argv[])
{
	int size;
	int where;
	void *ptr[BUFS];
	int i;
	int c;

	SyncMode = 0;
	BufferSize = 0;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'f':
				strcpy(Fname,optarg);
				break;
			case 's':
				BufferSize = atoi(optarg);
				break;
			case 'S':
				SyncMode = 1;
				break;
			default:
				fprintf(stderr,
					"unrecognzied command %c\n",(char)c);
				fprintf(stderr,
					"%s",Usage);
				fflush(stderr);
				exit(1);
		}
	}

	if(Fname[0] == 0) {
		fprintf(stderr,
			"must specify backing file name\n");
		fprintf(stderr, "%s",Usage);
		exit(1);
	}

	if(BufferSize == 0) {
		fprintf(stderr,
			"must specify size\n");
		fprintf(stderr, "%s",Usage);
		exit(1);
	}

	PmallocInit(Fname,BufferSize,SyncMode);
	

	for(i=0; i < BUFS; i++)
	{
		ptr[i] = NULL;
	}


	for(i=0; i < 50000; i++)
	{
		printf("iter: %d\n",i);
		where = (int)(RAND() * BUFS);

		if(ptr[where] == NULL)
		{
			size = (int)(RAND() * 25000);
			ptr[where] = Pmalloc(size);
			if(SyncMode == 1) {
				PmallocSyncObject(ptr[where],size);
			}
			if(ptr[where] == NULL)
			{
				fprintf(stderr,
				"malloc at iteration %d failed for size %d\n",
				i,size);
				fflush(stderr);
			}
		}
		else
		{
			Pfree(ptr[where]);
			ptr[where] = NULL;
		}
	}

	/*
	 * now -- free them
	 */
	for(i=0; i < BUFS; i++)
	{
		if(ptr[i] != NULL)
		{
			Pfree(ptr[i]);
			ptr[i] = NULL;
		}
	}

	PrintPmallocFreeList();
	printf("\n");

	printf("made it -- passed test\n");
	if(SyncMode == 0) {
		PmallocSync();
	}
	exit(0);
}



