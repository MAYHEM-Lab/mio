#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef THREAD
#include <pthread.h>
#endif


#include "my_malloc.h"

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

Mst *M_head;

extern unsigned char *MBuff; /* must be initialized before InitMyMalloc() */
extern unsigned long MBuffSize;

int Malloc_init = 0;

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
void InitMyMalloc()
{
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

	printf("InitMyMalloc() Initializing: Mst size: %ld, MBuff size: %ld\n",
			sizeof(Mst), MBuffSize);
	fflush(stdout);

	memset(MBuff,0,MBuffSize);

	/*
	 * stamp head of free list on first part of buffer
	 */
	M_head = (Mst *)MBuff;

	/*
	 * initialize to have one big free piece
	 */
	M_head->size = (MBuffSize - sizeof(Mst));
	M_head->next = NULL;
	M_head->prev = NULL;
	M_head->alloc = 0;
	/*
	 * use pointer arithmetic to get the buffer pointer
	 */
	M_head->buffer = (unsigned char *)(MBuff + sizeof(Mst));

	Malloc_init = 1;
#ifdef THREAD
	pthread_mutex_unlock(&MLock);
#endif

	return;
}

static void Coalesce()
{
	Mst *curr;
	Mst *prev;

#ifdef NO_COALESCE
	return;
#endif
#ifdef THREAD
	pthread_mutex_lock(&MLock);
#endif
	curr = M_head;


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
			}
			/*
			 * add its size and the size of the memory structure
			 * to the size of the previous block
			 */
			prev->size += (curr->size + sizeof(Mst));
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

void *MyMalloc(int size)
{
	Mst *curr;
	Mst *nextFree;
	int remainingSize;
	int lsize;

	/*
	 * do the init if it hasn't happened yet already
	 */
	InitMyMalloc();

#ifdef THREAD
	pthread_mutex_lock(&MLock);
#endif
	/*
	 * M_head is head of the free list
	 */
	curr = M_head;

	/*
	 * if M_head == NULL, all our space is allocated, return NULL
	 *
	 * this could happen if the first malloc allocates all of the
	 * available storage
	 */
	if(M_head == NULL)
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
		}
		if(curr->next != NULL)
		{
			curr->next->prev = curr->prev;
		}
		/*
		 * notice that M_head could become NULL here
		 */
		if(M_head == curr)
		{
			M_head = curr->next;
		}
		if(curr->alloc == 1) {
			fprintf(stderr,"realloc: 0x%lx\n",(long unsigned int)curr);
			fflush(stderr);
			exit(1);
		}
		curr->alloc = 1;
		curr->next = NULL;
		curr->prev = NULL;
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
	}
	else
	{
		nextFree->next = NULL;
	}

	if(curr->prev != NULL)
	{
		curr->prev->next = nextFree;
		nextFree->prev = curr->prev;
	}
	else
	{
		nextFree->prev = NULL;
	}

	/*
	 * if we are giving away M_head, there is nothing in front of
	 * it, and M_head should point to the new piece
	 */
	if(M_head == curr)
	{
		M_head = nextFree;
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
#ifdef THREAD
	pthread_mutex_unlock(&MLock);
#endif
	return((void *)curr->buffer);
}

void MyFree(void *i_buffer)
{
	Mst *toFree;	/* this is the Mst of the buffer coming back */
	Mst *curr;

	InitMyMalloc();

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
	curr = M_head;

	/*
	 * if the free list is empty, make this block the
	 * free list all by itself;
	 */
	if(curr == NULL)
	{
		M_head = toFree;
		toFree->next = NULL;
		toFree->prev = NULL;
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
		M_head = toFree;
#ifdef THREAD
		pthread_mutex_unlock(&MLock);
#endif
		Coalesce();
		return;
	}

	/*
	 * otherwise, look for the right spot
	 */
	while((curr != NULL) && (curr < toFree))
	{
		curr = curr->next;
	}

if((curr != NULL) && (curr == toFree)) {
printf("oops\n");
exit(1);
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
		curr = M_head;
		while(curr->next != NULL)
		{
			curr = curr->next;
		}
		curr->next = toFree;
		toFree->prev = curr;
		toFree->next = NULL;
#ifdef THREAD
		pthread_mutex_unlock(&MLock);
#endif
		Coalesce();
		return;
	}

	/*
	 * we have the right spot -- insert toFree immediately before
	 * curr
	 */
	if(curr != M_head) {
		curr->prev->next = toFree;
		toFree->prev = curr->prev;
	}
	toFree->next = curr;
	curr->prev = toFree;

#ifdef THREAD
	pthread_mutex_unlock(&MLock);
#endif

	Coalesce();
	return;
}



void PrintMyMallocFreeList()
{
	Mst *curr;

	if(M_head == NULL)
	{
		printf("Free List empty\n");
		return;
	}
	
#ifdef THREAD
	pthread_mutex_lock(&MLock);
#endif
	curr = M_head;

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
