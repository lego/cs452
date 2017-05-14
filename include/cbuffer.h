#ifndef __CBUFFER_H__
#define __CBUFFER_H__

/*
 * cbuffer.h
 */

#include <basic.h>

typedef struct {
  void **buffer;
  int max_size;
  int start;
  int size;
} cbuffer;

cbuffer cbuffer_create( void **buffer, int size );

// Returns status:
// 0 => OK
// -1 => ERROR: buffer full
int cbuffer_add(cbuffer *cbuffer, void *item);

// Returns status (ptr):
// 0 => OK
// -1 => ERROR: buffer empty
void *cbuffer_pop(cbuffer *cbuffer, int *status);


// Returns status (ptr):
// 0 => OK
// -1 => ERROR: buffer empty
int cbuffer_unpop(cbuffer *cbuffer, void *item);

bool cbuffer_full(cbuffer *cbuffer);
bool cbuffer_empty(cbuffer *cbuffer);
int cbuffer_size(cbuffer *cbuffer);

#endif
