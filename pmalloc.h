#if !defined(P_MALLOC_H)
#define P_MALLOC_H

/* mode == 0 => sync on close, 1 => sync on access */
void PmallocInit(char *fname, unsigned long size, int mode);
void *Pmalloc(int size);
void Pfree(void *buffer);
void PmallocSyncObject(unsigned char *addr, int size);
void PmallocSync();

void PrintPmallocFreeList();		/* optional for debugging */


#endif

