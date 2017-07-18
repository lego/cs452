#pragma once

#ifndef NO_KASSERT

#include <bwio.h>
#include <terminal.h>

#ifndef DEBUG_MODE

#define assert(_)
#include <debug.h>
#define KASSERT(a, msg, ...) do { if (!(a)) { \
  cleanup(false); bwputstr(COM2, SCROLL_DOWN_20); \
  bwprintf(COM2, "\n\r" RED_BG "KASSERT" RESET_ATTRIBUTES ": " msg "\n\rin %s:%d\n\r", ## __VA_ARGS__, __FILE__, __LINE__); \
  PrintBacktrace() \
  exit_kernel(); \
  } } while(0)

  // asm volatile ("swi #5"); cleanup();
  // bwputstr(COM2, "\n\r\n\r");
  // if (active_task) PrintAllTaskStacks(active_task->tid);
  // else PrintAllTaskStacks(-1);
  // bwprintf(COM2, "\n\r" RED_BG "KASSERT" RESET_ATTRIBUTES ": " msg "in \n\r%s:%d (%s)\n\r", ## __VA_ARGS__, __FILE__, __LINE__, __PRETTY_FUNCTION__);

#else
#include <assert.h>
#define KASSERT(a, msg, ...) do { if (!(a)) { \
  bwprintf(COM2, RED_BG "KASSERT: " RESET_ATTRIBUTES msg "\n\r%s:%d %s\n\r", ## __VA_ARGS__, __FILE__, __LINE__, __PRETTY_FUNCTION__); \
   } } while(0)
#endif

#else

#define KASSERT(a, msg, ...)

#endif
