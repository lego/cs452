#pragma once

/*
 * Experimental heap style alloc
 *
 * Currently doesn't seem to work, but presumed a result of static variables
 * not working. This will also be relatively hard to do 'right' (free-ing, defraging)
 *
 * The best use case would be one off allocations with no expected free (end of life)
 */

void allocator_init();

/**
 * allocates a chunk of memory
 * @param  size bytes of memory
 * @return      pointer to allocated memory
 *              if result is NULL, there is no more free memory
 */
void *alloc(unsigned int size);

/**
 * frees memory that was allocated
 * @return      status
 *              0 => OK
 *              1 => OK, but chunk is not re-allocatable
 *
 * Currently does nothing (not easy to implement)
 */
int jfree(void * ptr);

typedef struct {
  // serves as the pointer to the next free block
  // also when a chunk is allocated, it stores the size of the chunk
  void *next;
} memory_allocation_t;

#define RECORD_ALLOCATION_METRICS 1

#define MAX_HEAP_SIZE 1048576 // 2^20
#define MAX_REALLOCATION_SIZE 1024


// These are all metric tracking variables, which can help track
// the amount of memory pressure and other info
#if RECORD_ALLOCATION_METRICS
/* Allocation metrics */

#define remaining_memory (MAX_HEAP_SIZE - disbursed_size)

// size of memory disbursed, total
extern unsigned int disbursed_size;
// size of user memory disbursed, total
// this differs by not including memory_allocation_t overhead
#define disbursed_user_size (disbursed_size - sizeof(memory_allocation_t) * disbursed_count)
// count of succesfull alloc calls
extern unsigned int disbursed_count;
// count of unsuccessful alloc calls
extern unsigned int failed_count;
// size of disbursed reallocatable size (<= 1024 bytes)
// this is the net of how much has been given, including re-used memory
extern unsigned int disbursed_reallocatable_size;
// count of alloc used and given a one time realloctable chunk
extern unsigned int disbursed_reallocatable_count;
// amount of disbursed realloctable size, not including re-use
extern unsigned int reallocatable_size;
// count of disbursed realloctable allocs, not including re-use
extern unsigned int reallocatable_count;
// amount of re-used memory
#define reused_size (disbursed_reallocatable_size - reallocatable_size)
// count of  of re-used memory allocs
#define reused_count (disbursed_reallocatable_count - reallocatable_count)

/* Free metrics */
// amount of free-ed, non-reallocatable memory
extern unsigned int lost_memory_size;
extern unsigned int lost_memory_count;
// minimum and maximum sizes of lost memory
extern unsigned int lost_memory_min_size;
extern unsigned int lost_memory_max_size;
// returned memory
extern unsigned int memory_returned_size;
extern unsigned int memory_returned_count;
#endif
