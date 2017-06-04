#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdbool.h>

/*
 * Debug tooling
 */

#define DEBUG_LOGGING_ARM false
#define DEBUG_LOGGING_X86 true
// Enable various log_debug statements in the code
  #define DEBUG_SCHEDULER false
  #define DEBUG_CONTEXT_SWITCH false
  #define DEBUG_KERNEL_MAIN false
  #define DEBUG_TASK true
  #define DEBUG_SYSCALL false
  #define DEBUG_INTERRUPT false
  #define DEBUG_CLOCK_SERVER false
  #define DEBUG_NAMESERVER false

#define NOP do {} while(0)

#define RESET_ATTRIBUTES "\x1b" "[0m"
#ifdef DEBUG_MODE
#define GREY_FG "\x1b" "[37m"
#else
#define GREY_FG "\x1b" "[37m"
#endif

// Dangerous global use, but used only for debug lines
#include <kern/task_descriptor.h>
extern task_descriptor_t *active_task;

#if DEBUG_MODE
/**
 * This function is made to only be a breakpoint in GDB
 * By doing 'break debugger', we can stop on any calls to this
 * while we can also stub this to otherwise be a NOP
 */
void debugger();

#include <stdarg.h>
#include <stdio.h>
/**
 * Prints with a formatted string to STDERR for debugging output
 */
#if DEBUG_LOGGING_X86
#define log_debug(format, ...) fprintf(stderr, GREY_FG format RESET_ATTRIBUTES "\n\r", ## __VA_ARGS__)
#else
#define log_debug(format, ...) NOP
#endif

#include <assert.h>
#else

#define debugger() NOP
#include <bwio.h>
#if DEBUG_LOGGING_ARM
#define log_debug(format, ...) bwprintf(COM2, GREY_FG format RESET_ATTRIBUTES "\n\r", ## __VA_ARGS__)
#else
#define log_debug(format, ...) NOP
#endif
#define assert(x) ((void) (x))
#endif

void cleanup();
#define REDBOOT_LR 0x174c8
static inline void exit() {
  cleanup();
  asm volatile ("mov pc, %0" : : "r" (REDBOOT_LR));
}
#define KASSERT(a, msg, ...) do { if (!(a)) {bwprintf(COM2, "KASSERT: " msg "\n\r%s:%d %d\n\r", ## __VA_ARGS__, __FILE__, __LINE__, __PRETTY_FUNCTION__); exit();} } while(0)

#if DEBUG_SCHEDULER
#define log_scheduler_kern(format, ...) log_debug(" [-]{SC}  " format, ## __VA_ARGS__)
#define log_scheduler_task(format, ...) log_debug(" [-]{ST}  " format, ## __VA_ARGS__)
#else
#define log_scheduler_kern(format, ...) NOP
#define log_scheduler_task(format, ...) NOP
#endif

#if DEBUG_CONTEXT_SWITCH
#define log_context_swich(format, ...) log_debug(" [-]{CS} " format, ## __VA_ARGS__)
#else
#define log_context_swich(format, ...) NOP
#endif

#if DEBUG_KERNEL_MAIN
#define log_kmain(format, ...) log_debug(" [-]{M}  " format, ## __VA_ARGS__)
#else
#define log_kmain(format, ...) NOP
#endif

#if DEBUG_TASK
#define log_task(format, tid, ...) log_debug(" [%d]     " format, tid, ## __VA_ARGS__)
#else
#define log_task(format, tid, ...) NOP
#endif

#if DEBUG_SYSCALL
#define log_syscall(format, tid, ...) log_debug(" [%d]{SY} " format, tid, ## __VA_ARGS__)
#else
#define log_syscall(format, ...) NOP
#endif

#if DEBUG_INTERRUPT
#define log_interrupt(format, ...) log_debug("IN  " format, ## __VA_ARGS__)
#else
#define log_interrupt(format, ...) NOP
#endif

#if DEBUG_CLOCK_SERVER
#define log_clock_server(format, ...) log_task(format, ## __VA_ARGS__)
#else
#define log_clock_server(format, ...) NOP
#endif

#if DEBUG_NAMESERVER
#define log_nameserver(format, ...) log_debug("  [N]  " format, ## __VA_ARGS__)
#else
#define log_nameserver(format, ...) NOP
#endif

#endif
