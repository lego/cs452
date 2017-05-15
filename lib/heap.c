#include <basic.h>
#include <heap.h>

heap_t heap_create(heapnode_t *nodes, int size) {
  return (heap_t) {
           .nodes = nodes,
           .size = size,
           .len = 0
  };
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

// int main () {
//   heap_t *h = (heap_t *)calloc(1, sizeof (heap_t));
//   push(h, 3, "Clear drains");
//   push(h, 4, "Feed cat");
//   push(h, 5, "Make tea");
//   push(h, 1, "Solve RC tasks");
//   push(h, 2, "Tax return");
//   int i;
//   for (i = 0; i < 5; i++) {
//     printf("%s\n", pop(h));
//   }
//   return 0;
// }
