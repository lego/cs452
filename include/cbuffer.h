#ifndef __CBUFFER_H__
#define __CBUFFER_H__

/*
 * Circular buffer, using a pre-allocated buffer
 */

#include <basic.h>

typedef struct {
  void **buffer;
  int max_size;
  int start;
  int size;
} cbuffer_t;


/**
 * Initializes a circular buffer
 * @param  buffer memory to use
 * @param  size   of the buffer
 */
void cbuffer_init(cbuffer_t *cbuf, void **buffer, int size );


/**
 * Initializes a circular buffer
 * @param  buffer memory to use
 * @param  size   of the buffer
 */
cbuffer_t cbuffer_create( void **buffer, int size );

/**
 * Adds an item to the circular buffer
 * @return         status
 *                 0 => OK
 *                 -1 => ERROR: buffer full
 */
int cbuffer_add(cbuffer_t *cbuffer, void *item);

/**
 * Pops an item from the circular buffer
 * @return         status
 *                 0 => OK
 *                 -1 => ERROR: buffer empty
 */
void *cbuffer_pop(cbuffer_t *cbuffer, int *status);


/**
 * Unpops an item into the circular buffer
 * this adds the item back to the front of the buffer, providing peeking
 * @return         status
 *                 0 => OK
 *                 -1 => ERROR: buffer full
 */
int cbuffer_unpop(cbuffer_t *cbuffer, void *item);

// TODO: if possible these should be inline
/**
 * Checks if the buffer is full
 */
bool cbuffer_full(cbuffer_t *cbuffer);

/**
 * Checks if the buffer is empty
 */
bool cbuffer_empty(cbuffer_t *cbuffer);

/**
 * Gets the buffers size
 */
int cbuffer_size(cbuffer_t *cbuffer);

#endif
