#include <basic.h>
#include <heap.h>

heap_t heap_create(heapnode_t *nodes, int size) {
  heap_t heap;
  heap.nodes = nodes;
  heap.size = size;
  heap.len = 0;
  return heap;
}

int heap_size (heap_t *h) {
  return h->len;
}

int heap_push (heap_t *h, int priority, void *data) {
  if (h->len + 1 >= h->size) {
    return -1;
  }
  int i = h->len + 1;
  int j = i / 2;
  while (i > 1 && h->nodes[j].priority > priority) {
    h->nodes[i] = h->nodes[j];
    i = j;
    j = j / 2;
  }
  h->nodes[i].priority = priority;
  h->nodes[i].data = data;
  h->len++;
  return 0;
}

void *heap_pop (heap_t *h) {
  int i, j, k;
  if (!h->len) {
    return NULL;
  }
  void *data = h->nodes[1].data;
  h->nodes[1] = h->nodes[h->len];
  h->len--;
  i = 1;
  while (1) {
    k = i;
    j = 2 * i;
    if (j <= h->len && h->nodes[j].priority < h->nodes[k].priority) {
      k = j;
    }
    if (j + 1 <= h->len && h->nodes[j + 1].priority < h->nodes[k].priority) {
      k = j + 1;
    }
    if (k == i) {
      break;
    }
    h->nodes[i] = h->nodes[k];
    i = k;
  }
  h->nodes[i] = h->nodes[h->len + 1];
  return data;
}

int heap_peek_priority (heap_t *h) {
  if (!h->len) {
    return -1;
  }
  return h->nodes[1].priority;
}

void *heap_peek (heap_t *h) {
  if (!h->len) {
    return NULL;
  }
  return h->nodes[1].data;
}
