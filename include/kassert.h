#pragma once

#ifndef NO_KASSERT

#include <bwio.h>
#ifndef DEBUG_MODE
#include <debug.h>

#include <kern/task_descriptor.h>
#include <terminal.h>
#define KASSERT(a, msg, ...) do { if (!(a)) { \
  cleanup(); \
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
#include <stdlib.h>
#define KASSERT(a, msg, ...) do { if (!(a)) { \
  bwprintf(COM2, "KASSERT: " msg "\n\r%s:%d %s\n\r", ## __VA_ARGS__, __FILE__, __LINE__, __PRETTY_FUNCTION__); \
  exit(1); } } while(0)
#endif

#else

#define KASSERT(a, msg, ...)

#endif
