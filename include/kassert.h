#pragma once

#ifndef DEBUG_MODE
void cleanup();
#define REDBOOT_LR 0x174c8
static inline void exit() {
  cleanup();
  asm volatile ("mov pc, %0" : : "r" (REDBOOT_LR));
}
int lr;
int cpsr;

#include <bwio.h>

// TODO: it would be cool if our KASSERT was sensitive to user/kernel mode
// It can check the CPSR, and exit() if in the kernel or call
// ExitProgram if in user mode, to cleanly exit!
// Or maybe in kernel move it can jump to a label at the end of main?
#define KASSERT(a, msg, ...) do { if (!(a)) { \
  bwprintf(COM2, "KASSERT: " msg "\n\r%s:%d %s\n\r", ## __VA_ARGS__, __FILE__, __LINE__, __PRETTY_FUNCTION__); \
  bwprintf(COM2, "failed at lr=%x cpsr=%x\n\r", lr, cpsr); \
  exit();} } while(0)

// Light assert, no big deal
#define assert(x) ((void) (x))

#else
#include <stdlib.h>
#define KASSERT(a, msg, ...) do { if (!(a)) { \
  bwprintf(COM2, "KASSERT: " msg "\n\r%s:%d %s\n\r", ## __VA_ARGS__, __FILE__, __LINE__, __PRETTY_FUNCTION__); \
  exit(1); } } while(0)
#include <assert.h>
#endif
