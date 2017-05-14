/*
 * cbuffer.c - circular buffer
 */

#include <cbuffer.h>

cbuffer cbuffer_create(void **buffer, int size) {
  #ifdef DEBUG_MODE
  for (int i = 0; i < size; i++) {
    buffer[i] = 0;
  }
  #endif

  return (cbuffer) {
           .buffer = buffer,
           .max_size = size,
           .start = 0,
           .size = 0
  };
}

int cbuffer_add(cbuffer *cbuffer, void *item) {
  if (cbuffer->size >= cbuffer->max_size) {
    return -1;
  }

  int pos = cbuffer->start + cbuffer->size;
  // Do circular wrapping for pos
  if (pos >= cbuffer->max_size) {
    pos -= cbuffer->max_size;
  }

  cbuffer->buffer[pos] = item;
  cbuffer->size++;
  return 0;
}

void *cbuffer_pop(cbuffer *cbuffer, int *status) {
  if (cbuffer->size == 0) {
    if (status != NULL) *status = -1;
    return 0;
  }
  void *item = cbuffer->buffer[cbuffer->start];

  #ifdef DEBUG_MODE
  cbuffer->buffer[cbuffer->start] = 0;
  #endif

  cbuffer->size--;
  cbuffer->start++;
  // Do circular wrapping for start
  if (cbuffer->start >= cbuffer->max_size) {
    cbuffer->start = 0;
  }

  if (status != NULL) *status = 0;
  return item;
}

int cbuffer_unpop(cbuffer *cbuffer, void *item) {
  if (cbuffer->size >= cbuffer->max_size) {
    return -1;
  }

  int pos = cbuffer->start - 1;
  // Do circular wrapping for pos
  if (pos < 0) {
    pos += cbuffer->max_size;
  }

  cbuffer->buffer[pos] = item;
  cbuffer->start--;
  cbuffer->size++;
  return 0;
}

bool cbuffer_full(cbuffer *cbuffer) {
  return cbuffer->size == cbuffer->max_size;
}

bool cbuffer_empty(cbuffer *cbuffer) {
  return cbuffer->size == 0;
}

int cbuffer_size(cbuffer *cbuffer) {
  return cbuffer->size;
}
