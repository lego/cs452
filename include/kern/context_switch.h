#pragma once

#include <kern/context_switch.h>
#include <kern/kernel_request.h>
#include <kern/syscall.h>

/**
 * Executes a context switch, creating a stackframe with the syscall's arg1, arg2
 * and returning the syscalls retval
 *
 * All of those values will get handled by whatever assembly is backing the
 * actual context switch
 */
void context_switch(kernel_request_t *arg);

void context_switch_init();

void __asm_swi_handler();
void __asm_hwi_handler();
void __asm_start_task(void* task_sp, void* task_pc);
void __asm_switch_to_task(void* task_sp);
