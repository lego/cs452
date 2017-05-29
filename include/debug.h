#pragma once

#include <stdbool.h>
#include <terminal.h>

void cleanup();

void exit_kernel();


// Prints the stack trace, following memory (only on ARM)
void print_stack_trace(unsigned int fp, int lr);

void PrintTaskBacktrace(int tid);

// prints all task stacks, and puts the focused one on the end
void PrintAllTaskStacks(int focused_task);

#if DEBUG_MODE
#define PrintBacktrace()
#else
#define PrintBacktrace() do { unsigned int fp; asm volatile("mov %0, fp @ save fp" : "=r" (fp)); bwprintf(COM2, "Backtrace:\n\r"); print_stack_trace(fp, 0); } while(0);
#endif


/*
 * Debug tooling
 */

// A switch to limit output to only terminal information
// This must be set to false to use gtkterm
#if USE_PACKETS
  #define NONTERMINAL_OUTPUT true
#else
  #define NONTERMINAL_OUTPUT false
#endif

#define DEBUG_LOGGING_ARM true
#define DEBUG_LOGGING_X86 true
// Enable various log_debug statements in the code
  #define DEBUG_SCHEDULER false
  #define DEBUG_CONTEXT_SWITCH false
  #define DEBUG_KERNEL_MAIN false
  #define DEBUG_TASK false
  #define DEBUG_SYSCALL false
  #define DEBUG_INTERRUPT false
  #define DEBUG_CLOCK_SERVER false
  #define DEBUG_UART_SERVER false
  #define DEBUG_NAMESERVER false

#define NOP do {} while(0)

// Dangerous global use, but used only for debug lines
#include <kern/task_descriptor.h>
extern volatile task_descriptor_t *active_task;

#include <bwio.h>

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
#else

#define debugger() NOP
#if DEBUG_LOGGING_ARM
#define log_debug(format, ...) bwprintf(COM2, GREY_FG format RESET_ATTRIBUTES "\n\r", ## __VA_ARGS__)
#else
#define log_debug(format, ...) NOP
#endif
#endif



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

#if DEBUG_UART_SERVER
#define log_uart_server(format, ...) log_debug(" [U]  " format, ## __VA_ARGS__)
#else
#define log_uart_server(format, ...) NOP
#endif

#if DEBUG_NAMESERVER
#define log_nameserver(format, ...) log_debug("  [N]  " format, ## __VA_ARGS__)
#else
#define log_nameserver(format, ...) NOP
#endif
