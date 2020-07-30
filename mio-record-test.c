#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "mio.h"

#define ARGS "w:c:"
char *Usage = "usage: mio-record-test -w writefile -c record_count\n";
char Wfile[4096];

struct test_rec
{
	int data;
	int ndx;
	struct test_rec *next;
	struct test_rec *prev;
};

typedef struct test_rec TR;

int Count;

int main(int argc, char **argv)
{
	int c;
	MIO *mio;
	unsigned long int size;
	int err;
	TR *tr_array;
	TR *rec;
	TR *arec;
	int tr_head_ndx;
	int tr_tail_ndx;
	double r;
	int ndx;
	int i;


	Count = 0;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch (c) {
			case 'w':
				strncpy(Wfile,optarg,sizeof(Wfile));
				break;
			case 'c':
				Count = atoi(optarg);
				break;
			default:
				fprintf(stderr,
			"mio-record-test unrecognized command: %c\n",
					(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Wfile[0] == 0) {
		fprintf(stderr,"need a file\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	} 

	if(Count == 0) {
		fprintf(stderr,"need a record count\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	size = sizeof(TR)*Count;

	printf("attempting open for %s for write with size %ld\n",
			Wfile,
			size);
	mio = MIOOpen(Wfile,"a+",size);
	if(mio == NULL) {
		fprintf(stderr,"open text failed\n");
		exit(1);
	}
	printf("attempting open for %s complete\n",Wfile);

	/*
	 * set the address of the maped region to be an empty array of test
	 * records
	 */
	tr_array = (TR *)MIOAddr(mio);

	/*
	 * make sure it is zero filled
	 */
	memset(tr_array,0,size);

	/*
	 * create a randomized linked list
	 */
	tr_head_ndx = -1;
	tr_tail_ndx = -1;
	for(i=0; i < Count; i++) {
		r = drand48();
		/*
		 * pick a random index
		 */
		ndx = (int)(r * Count);
		/*
		 * extract address of random record
		 */
		rec = &(tr_array[ndx]);
		/*
		 * is it in use? if so, pick again
		 */
		while(rec->data != 0) {
			r = drand48();
			ndx = (int)(r * Count);
			rec = &(tr_array[ndx]);
		}

		/*
		 * store the current counter as the value (0 indicates free)
		 */
		rec->data = i+1;
		rec->ndx = ndx;
		/*
		 * link it into the list
		 */
		if(tr_head_ndx == -1) {
			/*
			 * first element in the list
			 */
			tr_head_ndx = ndx;
			tr_tail_ndx = ndx;
		} else { /* insert at the tail */
			arec = &(tr_array[tr_tail_ndx]);
			arec->next = rec;
			rec->prev = arec;
			tr_tail_ndx = ndx;
		}
	}
	printf("built random list of size %d\n",Count);

	/*
	 * print the list
	 */
	rec = &tr_array[tr_head_ndx];
	printf("head -- data: %d, ndx: %d\n",rec->data,tr_head_ndx);
	rec = rec->next;
	for(i=1; i < Count; i++) {
		printf("\tdata: %d, ndx: %d\n",rec->data,rec->ndx);
		rec = rec->next;
	}
		
	printf("attempting close for %s for write\n",
			Wfile);
	MIOClose(mio);

	printf("attempting reopen for %s ASSUMING that the data is mapped to the same addr\n",
			Wfile);

	mio = MIOOpen(Wfile,"a+",size);
	if(mio == NULL) {
		fprintf(stderr,"reopen failed\n");
		exit(1);
	}
	printf("attempting reopen for %s complete\n",Wfile);
	tr_array = (TR *)MIOAddr(mio);
	
	printf("attempting to print the list after reopen\n");
	rec = &tr_array[tr_head_ndx];
	printf("head -- data: %d, ndx: %d\n",rec->data,tr_head_ndx);
	rec = rec->next;
	for(i=1; i < Count; i++) {
		printf("\tdata: %d, ndx: %d\n",rec->data,rec->ndx);
		rec = rec->next;
	}

	printf("attempting reclose\n");
	MIOClose(mio);
	printf("reclose complete\n");

	return(0);
}

