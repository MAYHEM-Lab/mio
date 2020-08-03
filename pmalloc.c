/*
 * persistent malloc
 * version of first-fit that is self-describing
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef THREAD
#include <pthread.h>
#endif


#include "mio.h"
#include "pmalloc.h"

#define ALIGNSIZE (sizeof (double))

#ifdef THREAD
pthread_mutex_t MLock = PTHREAD_MUTEX_INITIALIZER;
#endif


struct malloc_stc
{
	struct malloc_stc *next;
	struct malloc_stc *prev;
	int size;
	int alloc;
	unsigned char *buffer;
};

typedef struct malloc_stc Mst;

struct malloc_meta
{
	Mst *M_head;
	unsigned long MBuffSize;
	unsigned char *start;
	int mode;
};

typedef struct malloc_meta Mmeta;

int Malloc_init = 0;
Mmeta *Malloc_meta;	/* global ID for buffer */

static void PrintMst(Mst *m)
{
	printf("block: 0x%lx\n",(long unsigned int)m);
	printf("\tsize: %d\n",m->size);
//	printf("\talloc: %d\n",m->alloc);
	printf("\tnext: 0x%lx\n",(long unsigned int)(m->next));
	printf("\tprev: 0x%lx\n",(long unsigned int)(m->prev));
	printf("\tbuffer: 0x%lx\n",(long unsigned int)(m->buffer));

	return;
}
void PmallocInit(char *fname, unsigned long size, int mode)
{
	MIO *mio;
	Mmeta *meta;
	unsigned char *buff;
	unsigned long bsize;

#ifdef THREAD
	pthread_mutex_lock(&MLock);
#endif
	if(Malloc_init == 1)
	{
#ifdef THREAD
		pthread_mutex_unlock(&MLock);
#endif
		return;
	}

	bsize = size  + sizeof(Mmeta);
	/*
 	 * map a buffer backed by a file
	 */
	mio = MIOOpen(fname,"a+",bsize);
	if(mio == NULL) {
		fprintf(stderr,
			"PmallocInit: failed with file %s and size %ld\n",
				fname,
				size);
		fflush(stderr);
		return;
	}
	buff = MIOAddr(mio);
	memset(buff,0,bsize);

	/*
 	 * set up meta data
 	 */
	meta = (Mmeta *)buff;

	/*
	 * stamp head of free list on first part of buffer
	 */
	meta->start = buff  + sizeof(Mmeta);
	meta->M_head = (Mst *)meta->start;
	meta->MBuffSize = size;

	/*
	 * initialize to have one big free piece
	 */
	meta->M_head->size = size;
	meta->M_head->next = NULL;
	meta->M_head->prev = NULL;
	meta->M_head->alloc = 0;
	/*
	 * use pointer arithmetic to get the buffer pointer
	 */
	meta->M_head->buffer = (unsigned char *)meta->start+sizeof(Mst);

	Malloc_meta = meta;
	Malloc_init = 1;
	meta->mode = mode;
	if(meta->mode == 1) {
		MIOSync(mio);
	}
#ifdef THREAD
	pthread_mutex_unlock(&MLock);
#endif

	return;
}

static void Pcoalesce()
{
	Mst *curr;
	Mst *prev;
	Mmeta *meta = Malloc_meta;

#ifdef NO_COALESCE
	return;
#endif
#ifdef THREAD
	pthread_mutex_lock(&MLock);
#endif
	curr = meta->M_head;


	/*
	 * paranoid -- should never happen
	 */
	if(curr == NULL)
	{
#ifdef THREAD
		pthread_mutex_unlock(&MLock);
#endif
		return;
	}

	/*
	 * if the next pointer is NULL, the list is
	 * completely coalesced
	 */
	if(curr->next == NULL)
	{
#ifdef THREAD
		pthread_mutex_unlock(&MLock);
#endif
		return;
	}

	/*
	 * otherwise, walk down the list and try to
	 * coalesce with the previous block
	 */ 
	prev = curr;
	curr = curr->next;
	while(curr != NULL)
	{
		/*
		 * if the pevious free buffer ends at the start
		 * of the current buffer, merge current buffer
		 * and previous one
		 */
		if((prev->buffer + prev->size) == (unsigned char *)curr)
		{
			/*
			 * unlink curr from list
			 */
			prev->next = curr->next;
			if(curr->next != NULL)
			{
				curr->next->prev = prev;
				/* sync because it has changed */
				if(meta->mode == 1) {
					MIOSyncObject(curr->next,sizeof(Mst));
				}
			}
			/*
			 * add its size and the size of the memory structure
			 * to the size of the previous block
			 */
			prev->size += (curr->size + sizeof(Mst));
			/*
			 * sync curr and prev since they have been altered
			 */
			if(meta->mode == 1) {
				MIOSyncObject(prev,sizeof(Mst));
			}
			/*
			 * bump curr along, but leave prev where it is
			 * since it is still the prev of the next slot
			 */
			curr = prev->next;
			continue;
		}
		/*
		 * otherwise, move them both along and try again
		 */
		prev = curr;
		curr = curr->next;
	}
#ifdef THREAD
	pthread_mutex_unlock(&MLock);
#endif

	return;
}

void *Pmalloc(int size)
{
	Mst *curr;
	Mst *nextFree;
	Mmeta *meta = Malloc_meta;
	int remainingSize;
	int lsize;

	if(Malloc_init == 0) {
		fprintf(stderr,
			"must call PmallocInit() before calling Pmalloc\n");
		fflush(stderr);
		exit(1);
	}

#ifdef THREAD
	pthread_mutex_lock(&MLock);
#endif
	/*
	 * M_head is head of the free list
	 */
	curr = meta->M_head;

	/*
	 * if M_head == NULL, all our space is allocated, return NULL
	 *
	 * this could happen if the first malloc allocates all of the
	 * available storage
	 */
	if(meta->M_head == NULL)
	{
#ifdef THREAD
	pthread_mutex_unlock(&MLock);
#endif
		return(NULL);
	}

	/*
	 * round up to multiple of four bytes to prevent alignment problems
	 */
	lsize = size / ALIGNSIZE;
	if((lsize * ALIGNSIZE) != size)
	{
		lsize = (size + ALIGNSIZE) / ALIGNSIZE;
		lsize = lsize * ALIGNSIZE;
	}
	else
	{
		lsize = size;
	}
	/*
	 * implement first fit
	 */
	while((curr != NULL) && (curr->size < lsize))
	{
		curr = curr->next;
	}

	/*
	 * here, either we found a chunk big enough (curr != NULL)
	 * or there wasn't a chunk (curr == NULL)
	 */
	if(curr == NULL)
	{
#ifdef THREAD
		pthread_mutex_unlock(&MLock);
#endif
		return(NULL);
	}

	/*
	 * okay -- we found the chunk, and curr points to it
	 *
	 * we need to allocate only as much as is requested.  To do
	 * so, we must split curr into the aloocated piece of size "size"
	 * and a free piece which contains the remaining free piece.  We
	 * also need to keep the free list consistent.
	 */

	/*
	 * Step 1: calculate how much space in curr is left after size
	 * is allocated
	 *
	 * there must be enough left to hold a complete Mst and at least
	 * one 4 bytes of storage (to keep from having alignment problems)
	 */
	remainingSize = curr->size - lsize;
	/*
	 * if what is left isn't big enough to hold an Mst + 4 bytes,
	 * don't split
	 */
	if(remainingSize <= (sizeof(Mst) + ALIGNSIZE))
	{
		/*
		 * pull curr out of the free list
		 *
		 * need to test in case curr is either head or tail of
		 * list or both
		 */
		if(curr->prev != NULL)
		{
			curr->prev->next = curr->next;
			if(meta->mode == 1) {
				MIOSyncObject(curr->prev,sizeof(Mst));
			}
		}
		if(curr->next != NULL)
		{
			curr->next->prev = curr->prev;
			if(meta->mode == 1) {
				MIOSyncObject(curr->next,sizeof(Mst));
			}
		}
		/*
		 * notice that M_head could become NULL here
		 */
		if(meta->M_head == curr)
		{
			meta->M_head = curr->next;
			if(meta->mode == 1) {
				MIOSyncObject(meta,sizeof(Mmeta));
			}
		}
		if(curr->alloc == 1) {
			fprintf(stderr,"realloc: 0x%lx\n",(long unsigned int)curr);
			fflush(stderr);
			exit(1);
		}
		curr->alloc = 1;
		curr->next = NULL;
		curr->prev = NULL;
		if(meta->mode == 1) {
			MIOSyncObject(curr,sizeof(Mst));
		}
#ifdef THREAD
		pthread_mutex_unlock(&MLock);
#endif
		return((void *)curr->buffer);
	}

	/*
	 * here, we need to split the free block that curr points to
	 *
	 * stamp nextFree into the free space just after the allocated
	 * space
	 */
	nextFree = (Mst *)((unsigned char *)curr->buffer + lsize);
	/*
	 * set the new size -- the available size is the remaining
	 * amount minus the amount of space necessary to hold the
	 * size of the data structure
	 */
	nextFree->size = curr->size - (lsize + sizeof(Mst));
	/*
	 * point nextFree buffer at its new space
	 */
	nextFree->buffer = (unsigned char *)nextFree + sizeof(Mst);
	/*
	 * now, link nextFree into the free list -- be careful about
	 * end cases
	 */
	nextFree->alloc = 0;
	if(curr->next != NULL)
	{
		curr->next->prev = nextFree;
		nextFree->next = curr->next;
		if(meta->mode == 1) {
			MIOSyncObject(curr->next,sizeof(Mst));
			MIOSyncObject(nextFree,sizeof(Mst));
		}
	}
	else
	{
		nextFree->next = NULL;
		if(meta->mode == 1) {
			MIOSyncObject(nextFree,sizeof(Mst));
		}
	}

	if(curr->prev != NULL)
	{
		curr->prev->next = nextFree;
		nextFree->prev = curr->prev;
		if(meta->mode == 1) {
			MIOSyncObject(curr->prev,sizeof(Mst));
			MIOSyncObject(nextFree,sizeof(Mst));
		}
	}
	else
	{
		nextFree->prev = NULL;
		if(meta->mode == 1) {
			MIOSyncObject(nextFree,sizeof(Mst));
		}
	}

	/*
	 * if we are giving away M_head, there is nothing in front of
	 * it, and M_head should point to the new piece
	 */
	if(meta->M_head == curr)
	{
		meta->M_head = nextFree;
		if(meta->mode == 1) {
			MIOSyncObject(meta,sizeof(Mmeta));
		}
	}

	/*
	 * trim curr to the requested size rounded up
	 */
	curr->size = lsize;

	/*
	 * dump the next and prev pointers for safety
	 */
	curr->next = NULL;
	curr->prev = NULL;

	curr->alloc = 1;
	if(meta->mode == 1) {
		MIOSyncObject(curr,sizeof(Mst));
	}
#ifdef THREAD
	pthread_mutex_unlock(&MLock);
#endif
	return((void *)curr->buffer);
}

void Pfree(void *i_buffer)
{
	Mst *toFree;	/* this is the Mst of the buffer coming back */
	Mst *curr;
	Mmeta *meta = Malloc_meta;

	if(Malloc_init == 0) {
		fprintf(stderr,
			"must call PmallocInit before calling Pfree\n");
		fflush(stderr);
		exit(1);
	}

	if(i_buffer == NULL) {
		return;
	}

#ifdef THREAD
	pthread_mutex_lock(&MLock);
#endif
	/*
	 * get the Mst from the incoming buffer value -- it will always be
	 * an Mst back from the pointer
	 */
	toFree = (Mst *)(i_buffer - sizeof(Mst));


	if(toFree->alloc == 0) {
		fprintf(stdout,"refree: 0x%lx\n",(long unsigned int)toFree);
		fflush(stdout);
	}
	toFree->alloc = 0;

	/*
	 * keep the free list in sorted order -- it makes compacting
	 * easier
	 */
	curr = meta->M_head;

	/*
	 * if the free list is empty, make this block the
	 * free list all by itself;
	 */
	if(curr == NULL)
	{
		meta->M_head = toFree;
		toFree->next = NULL;
		toFree->prev = NULL;
		if(meta->mode == 1) {
			MIOSyncObject(meta,sizeof(Mmeta));
			MIOSyncObject(toFree,sizeof(Mst));
		}
#ifdef THREAD
		pthread_mutex_unlock(&MLock);
#endif
		return;
	}
	
	/*
	 * if the address of toFree is less than curr, toFree should be
	 * first on the list
	 */
	if(curr > toFree)
	{
		toFree->next = curr;
		curr->prev = toFree;
		toFree->prev = NULL;
		meta->M_head = toFree;
		if(meta->mode == 1) {
			MIOSyncObject(toFree,sizeof(Mst));
			MIOSyncObject(curr,sizeof(Mst));
			MIOSyncObject(meta,sizeof(Mmeta));
		}
#ifdef THREAD
		pthread_mutex_unlock(&MLock);
#endif
		Pcoalesce();
		return;
	}

	/*
	 * otherwise, look for the right spot
	 */
	while((curr != NULL) && (curr < toFree))
	{
		curr = curr->next;
	}

	/*
	 * here, either we have fallen off the end, in which case
	 * toFree should be inserted at the end, or we have the right
	 * spot
	 */
	if(curr == NULL)
	{
		/*
		 * find the end again (can be made more efficient)
		 */
		curr = meta->M_head;
		while(curr->next != NULL)
		{
			curr = curr->next;
		}
		curr->next = toFree;
		toFree->prev = curr;
		toFree->next = NULL;
		if(meta->mode == 1) {
			MIOSyncObject(curr,sizeof(Mst));
			MIOSyncObject(toFree,sizeof(Mst));
		}
#ifdef THREAD
		pthread_mutex_unlock(&MLock);
#endif
		Pcoalesce();
		return;
	}

	/*
	 * we have the right spot -- insert toFree immediately before
	 * curr
	 */
	if(curr != meta->M_head) {
		curr->prev->next = toFree;
		toFree->prev = curr->prev;
		if(meta->mode == 1) {
			MIOSyncObject(curr->prev,sizeof(Mst));
			MIOSyncObject(toFree,sizeof(Mst));
		}
	}
	toFree->next = curr;
	curr->prev = toFree;
	if(meta->mode == 1) {
		MIOSyncObject(toFree,sizeof(Mst));
		MIOSyncObject(curr,sizeof(Mst));
	}


#ifdef THREAD
	pthread_mutex_unlock(&MLock);
#endif

	Pcoalesce();
	return;
}



void PrintPmallocFreeList()
{
	Mst *curr;
	Mmeta *meta = Malloc_meta;

	if(meta->M_head == NULL)
	{
		printf("Free List empty\n");
		return;
	}
	
#ifdef THREAD
	pthread_mutex_lock(&MLock);
#endif
	curr = meta->M_head;

	while(curr != NULL)
	{
		PrintMst(curr);
		curr = curr->next;
	}

#ifdef THREAD
	pthread_mutex_unlock(&MLock);
#endif
	return;
}

void PmallocSync()
{
	Mmeta *meta = Malloc_meta;

	if(Malloc_init == 0) {
		return;
	}

	MIOSyncObject(meta,meta->MBuffSize+sizeof(Mmeta));
	return;
}
