#ifndef __HEAP__H
#define __HEAP__H

typedef struct {
  int priority;
  void *data;
} heapnode_t;

typedef struct {
  heapnode_t *nodes;
  int len;
  int size;
} heap_t;

/**
 * Initializes a heap
 * @param  nodes memory buffer to use for the heap data
 * @param  size  of the memory buffer
 */
heap_t heap_create(heapnode_t *nodes, int size);

// TODO: if possible these should be inline
/**
 * Pushes an item to the heap
 * @param  priority of the item
 * @param  data     ptr to insert into the heap
 * @return          status
 *                  0 => OK
 *                  -1 => ERROR: heap full
 */
int heap_push(heap_t *h, int priority, void *data);

/**
 * Pops the highest priority item off the heap
 */
void *heap_pop(heap_t *h);

/**
 * Gets the current size of the heap
 */
int heap_size (heap_t *h);

#endif
