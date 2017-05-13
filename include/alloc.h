#ifndef __ALLOC_H__
#define __ALLOC_H__

#include <basic.h>


void *alloc(unsigned int size);

// Returns status:
// 0 => OK
int free(void * ptr);

#endif
