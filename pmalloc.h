#if !defined(P_MALLOC_H)
#define P_MALLOC_H

void PmallocInit(char *fname, unsigned long size);
void *Pmalloc(int size);
void Pfree(void *buffer);

void PrintPmallocFreeList();		/* optional for debugging */


#endif

