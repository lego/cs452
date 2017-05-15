#ifndef __HEAP__H
#define __HEAP__H

typedef struct {
  int priority;
  void *data;
} node_t;

typedef struct {
  node_t *nodes;
  int len;
  int size;
} heap_t;

heap_t heap_create(node_t *nodes, int size);

// TODO: inline these functions maybe?

// Returns status:
// 0 => OK
// -1 => Out of space
int heap_push (heap_t *h, int priority, void *data);

void *heap_pop (heap_t *h);
int heap_size (heap_t *h);

#endif
