#include <stddef.h>

#include <bwio.h>
#include <alloc.h>
#include <kernel.h>
#include <stdint.h>
#include <stdlib/string.h>

#define PRINT_INT(var) bwprintf(COM2, "%s=%d\n\r", #var, var)
#define PRINT_UINT(var) bwprintf(COM2, "%s=%u\n\r", #var, var)

void malloc_test_task() {
  bwprintf(COM2, "==== Starting alloc test\n\r");
  bwprintf(COM2, " -- Attempting bzero on memory filled with 0xFF. Logging results: \n\r");
  char mem[128];
  int i;
  for (i = 0; i < 128; i++) {
    mem[i] = 0xFF;
  }
  // ensure memory is unaligned and not beginning at memstart
  // push to be 7bit aligned
  char *offset_mem = mem;
  char k;
  if (((uint32_t) offset_mem & 3) != 3) offset_mem += 1;
  else offset_mem += 4;
  memset(offset_mem, 0, 112);
  bwprintf(COM2, " -- Memory zerod from mem=%x start=%x end=%x actual_end=%x\n\r", (unsigned int) mem, (unsigned int) offset_mem, (unsigned int) offset_mem + 112, (unsigned int) mem + 128);
  for (i = 0; i < 128; i += 8) {
    if (i % 64 == 0 && i > 0) bwprintf(COM2, "\n\r");
    bwprintf(COM2, "%x%x", *(unsigned int *)(mem+i), *(unsigned int *)(mem+i+4));
  }
  bwprintf(COM2, "\n\r");

  void * data;
  int result;
  bwprintf(COM2, " -- Attempting to malloc 32 bytes\n\r");
  data = Malloc(32);
  bwprintf(COM2, " addr=%x\n\r", (unsigned int) data);

  memory_allocation_t *chunk = (memory_allocation_t *) (data - sizeof(memory_allocation_t));
  bwprintf(COM2, " internal_size=%u addr=%x\n\r", (unsigned int) chunk->next, (unsigned int) chunk);


  bwprintf(COM2, " -- Attempting to free memory\n\r");
  result = Free(data);
  bwprintf(COM2, " result=%d\n\r", result);
  bwprintf(COM2, " -- Attempting to malloc 32 bytes\n\r");
  data = Malloc(32);
  bwprintf(COM2, " addr=%x\n\r", (unsigned int) data);

  bwprintf(COM2, " -- Alloc metrics\n\r");

  // #define remaining_memory (MAX_HEAP_SIZE - disbursed_size)
  PRINT_UINT(disbursed_size);
  PRINT_UINT((unsigned int) disbursed_user_size);
  PRINT_UINT(disbursed_count);
  PRINT_UINT(failed_count);
  PRINT_UINT(disbursed_reallocatable_size);
  PRINT_UINT(disbursed_reallocatable_count);
  PRINT_UINT(reallocatable_size);
  PRINT_UINT(reallocatable_count);
  PRINT_UINT((unsigned int) reused_size);
  PRINT_UINT((unsigned int)reused_count);

  bwprintf(COM2, "\n\r -- Free metrics\n\r");
  PRINT_UINT(lost_memory_size);
  PRINT_UINT(lost_memory_count);
  PRINT_UINT(lost_memory_min_size);
  PRINT_UINT(lost_memory_max_size);
  PRINT_UINT(memory_returned_size);
  PRINT_UINT(memory_returned_count);

  ExitKernel();
}
