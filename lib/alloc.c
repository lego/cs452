#include <basic.h>
#include <debug.h>
#include <alloc.h>
#include <stdlib/string.h>

#if RECORD_ALLOCATION_METRICS
/* Allocation metrics */
unsigned int disbursed_size;
unsigned int disbursed_count;
unsigned int failed_count;
unsigned int disbursed_reallocatable_size;
unsigned int disbursed_reallocatable_count;
unsigned int reallocatable_size;
unsigned int reallocatable_count;

/* Free metrics */
unsigned int lost_memory_size;
unsigned int lost_memory_count;
unsigned int lost_memory_min_size;
unsigned int lost_memory_max_size;
unsigned int memory_returned_size;
unsigned int memory_returned_count;
#endif

static char memory[MAX_HEAP_SIZE];
static unsigned int next_location;
static void *next_free_chunk[MAX_REALLOCATION_SIZE];

void allocator_init() {
  next_location = 0;
  memset(next_free_chunk, 0, MAX_REALLOCATION_SIZE);

  #if RECORD_ALLOCATION_METRICS
  disbursed_size = 0;
  disbursed_count = 0;
  failed_count = 0;
  disbursed_reallocatable_size = 0;
  disbursed_reallocatable_count = 0;
  reallocatable_size = 0;
  reallocatable_count = 0;
  lost_memory_size = 0;
  lost_memory_count = 0;
  lost_memory_min_size = 9999999;
  lost_memory_max_size = 0;
  memory_returned_size = 0;
  memory_returned_count = 0;
  #endif
}

void *alloc(unsigned int size) {
  memory_allocation_t *chunk;
  unsigned int actual_size = size + sizeof(memory_allocation_t);

  // re-allocate freed memory of the same block size
  if (next_location + actual_size >= MAX_HEAP_SIZE) {
    #if RECORD_ALLOCATION_METRICS
    failed_count++;
    #endif
    return NULL;
  } else if (actual_size <= MAX_REALLOCATION_SIZE && next_free_chunk[actual_size - 1] != NULL) {
    chunk = (memory_allocation_t *) next_free_chunk[actual_size - 1];
    next_free_chunk[actual_size - 1] = chunk->next;
  } else {
    // FIXME: align memory
    chunk = (void *) memory + next_location;
    next_location += actual_size;

    #if RECORD_ALLOCATION_METRICS
    if (actual_size <= MAX_REALLOCATION_SIZE) {
      reallocatable_size += actual_size;
      reallocatable_count++;
    }
    #endif
  }

  #if RECORD_ALLOCATION_METRICS
  disbursed_size += actual_size;
  disbursed_count++;

  if (actual_size <= MAX_REALLOCATION_SIZE) {
    disbursed_reallocatable_size += actual_size;
    disbursed_reallocatable_count++;
  }
  #endif

  chunk->next = (void *) actual_size;
  return ((char *) chunk) + sizeof(memory_allocation_t);
}

int jfree(void *ptr) {
  memory_allocation_t *chunk = (memory_allocation_t *) (ptr - sizeof(memory_allocation_t));
  unsigned int size = (unsigned int) chunk->next;
  if (size <= MAX_REALLOCATION_SIZE) {
    chunk->next = next_free_chunk[size - 1];

    #if RECORD_ALLOCATION_METRICS
    memory_returned_size += size;
    memory_returned_count++;
    #endif

    next_free_chunk[size - 1] = chunk;
    return 0;
  } else {
    #if RECORD_ALLOCATION_METRICS
    lost_memory_size += size;
    lost_memory_count++;

    if (size < lost_memory_min_size) lost_memory_min_size = size;
    if (size > lost_memory_max_size) lost_memory_max_size = size;
    #endif

    return 1;
  }
}
