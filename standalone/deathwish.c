#include <io.h>
#include <bwio.h>

#define DEATHWISH_LOCATION (unsigned int *) 0x280000

void __cyg_profile_func_enter (void *, void *) __attribute__((no_instrument_function));
void __cyg_profile_func_exit (void *, void *) __attribute__((no_instrument_function));

void __cyg_profile_func_enter (void *func,  void *caller)
{
}

void __cyg_profile_func_exit (void *func, void *caller)
{
}

void *active_task;
void *main_fp;
void cleanup() {}
void PrintAllTaskStacks() {}

int main() {
  io_init();
  unsigned int deathwish_val = *DEATHWISH_LOCATION;
  bwprintf(COM2, "Deathwish value=%08x", deathwish_val);
  bwsetfifo(COM2, ON);
}
