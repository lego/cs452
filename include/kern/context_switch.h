#ifndef __CONTEXT_SWITCH_H__
#define __CONTEXT_SWITCH_H__

/*
 * Internal context switch bits
 *
 * These defines are the different integers associated with software interrupts
 * to denote which syscall is being made
 */
typedef int syscall_t;
#define SYSCALL_MY_TID (syscall_t) 1
#define SYSCALL_MY_PARENT_TID (syscall_t) 2
#define SYSCALL_CREATE (syscall_t) 3
#define SYSCALL_PASS (syscall_t) 4
#define SYSCALL_EXIT (syscall_t) 5

/**
 * Executes a context switch, creating a stackframe with the syscall's arg1, arg2
 * and returning the syscalls retval
 *
 * All of those values will get handled by whatever assembly is backing the
 * actual context switch
 */
void *context_switch(syscall_t call_no, int arg1, void *arg2);

void context_switch_init();

void __asm_swi_handler();
void __asm_start_task(void* task_sp, void* task_pc);
void __asm_switch_to_task(void* task_sp);

#endif
