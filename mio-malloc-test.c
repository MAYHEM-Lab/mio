#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "mio.h"
#include "my_malloc.h"

#define ARGS "w:c:"
char *Usage = "usage: mio-malloc-test -w writefile -c record_count\n";
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
/*
 * next two are for my_malloc
 */
unsigned char *MBuff;
unsigned long MBuffSize;

int main(int argc, char **argv)
{
	int c;
	MIO *mio;
	unsigned long int size;
	int err;
	TR *rec;
	TR *head;
	TR *tail;
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

	MBuffSize = 100*sizeof(TR)*Count; /* make it 100 times as large just for good measure */

	printf("attempting open for %s for write with size %ld\n",
			Wfile,
			size);
	mio = MIOOpen(Wfile,"a+",MBuffSize);
	if(mio == NULL) {
		fprintf(stderr,"open text failed\n");
		exit(1);
	}
	printf("attempting open for %s complete\n",Wfile);

	/*
	 * set the address of the maped region to be the buffer needed by
	 * my_malloc
	 */
	MBuff = MIOAddr(mio);

	/*
	 * initialize my_malloc
	 */
	InitMyMalloc();

	for(i=0; i < Count; i++) {
		rec = MyMalloc(sizeof(TR));
		if(rec == NULL) {
			printf("malloc of record failed\n");
			exit(1);
		}

		/*
		 * store the current counter as the value
		 */
		rec->data = i+1;
		/*
		 * link it into the list
		 */
		if(i == 0) {
			/*
			 * first element in the list
			 */
			head = rec;
			tail = rec;
			rec->next = NULL;
			rec->prev = NULL;
		} else { /* insert at the tail */
			tail->next = rec;
			rec->prev = tail;
			tail = rec;
		}
	}
	printf("built random list of size %d\n",Count);

	/*
	 * print the list
	 */
	rec = head;
	printf("head -- data: %d\n",rec->data);
	rec = rec->next;
	for(i=1; i < Count; i++) {
		printf("\tdata: %d\n",rec->data);
		rec = rec->next;
	}
		
	printf("attempting close for %s for write\n",
			Wfile);
	MIOClose(mio);

	printf("attempting reopen for %s ASSUMING that the data is mapped to the same addr\n",
			Wfile);

	mio = MIOOpen(Wfile,"a+",MBuffSize);
	if(mio == NULL) {
		fprintf(stderr,"reopen failed\n");
		exit(1);
	}
	printf("attempting reopen for %s complete\n",Wfile);
	MBuff = MIOAddr(mio);
	
	printf("attempting to print the list after reopen\n");
	rec = head;
	printf("head -- data: %d\n",rec->data);
	rec = rec->next;
	for(i=1; i < Count; i++) {
		printf("\tdata: %d\n",rec->data);
		rec = rec->next;
	}

	/*
	 * free the list
	 */
	printf("attempting to free the list\n");
	rec = head;
	for(i=0; i < Count; i++) {
		if(rec->next != NULL) {
			rec = rec->next;
			MyFree(rec->prev);
		} else {
			MyFree(rec);
		}
	}

	printf("attempting reclose\n");
	MIOClose(mio);
	printf("reclose complete\n");

	return(0);
}

