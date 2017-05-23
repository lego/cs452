#include <basic.h>
#include <bwio.h>
#include <kern/context.h>
#include <kern/context_switch.h>
#include <kern/scheduler.h>
#include <kern/task_descriptor.h>

void* kernelSp = 0;

typedef int (*interrupt_handler)(int);

void context_switch_init() {
  kernelSp = 0;
  *((interrupt_handler*)0x28) = (interrupt_handler)&__asm_swi_handler;
}

void __asm_swi_handler();
void __asm_switch_to_task(void* task_sp);
void __asm_start_task(void* task_sp, void* task_pc);

asm (
"\n"
// export of the function
".global __asm_start_task\n"
".global __asm_switch_to_task\n"

"__asm_switch_to_task:\n\t"
  // save the return-to-kernel lr + registers
  "stmfd sp!, {r4-r12, lr}\n\t"

  // in system mode
  "msr cpsr_c, #223\n\t"
    // switch to argument stack pointer
    "mov sp, r0\n\t"

    // recover task registers
    "ldmfd sp!, {r1, r2, r4-r12, lr}\n\t"
    "msr spsr_c, r1\n\t"
  "msr cpsr_c, #211\n\t"

  "mov lr, r2\n\t"

  // begin execution of task
  "movs pc, lr\n\t"

"\n"
"__asm_swi_handler:\n\t"
  // in system mode
  "msr cpsr_c, #223\n\t"
    // store task state and lr
    "stmfd sp!, {r4-r12, lr}\n\t"
  "msr cpsr_c, #211\n\t"

  "mrs r4, spsr\n\t"
  // save return-to-user lr to r5 so it's not overwritten by r0-r3
  "mov r5, lr\n\t"

  // in system mode
  "msr cpsr_c, #223\n\t"
    "stmfd sp!, {r4, r5}\n\t"
    "mov r4, sp\n\t"
  "msr cpsr_c, #211\n\t"

  // set up args for syscall
  "mov r1, r0\n\t" // kernel request ptr
  "mov r0, r4\n\t" // task stack pointer

  // handle syscall
  "bl __syscall(PLT)\n\t"

  // load the return-to-kernel lr + registers
  "ldmfd sp!, {r4-r12, lr}\n\t"

  // return from scheduler_activate_task in kernel
  "mov pc, lr\n\t"

"\n"
"__asm_start_task:\n\t"
  // store return-to-kernel lr + registers
  "stmfd sp!, {r4-r12, lr}\n\t"

  // in system mode
  "msr cpsr_c, #223\n\t"
    // switch to new stack
    "mov sp, r0\n\t"

    // Store Exit function as the return place
    "ldr r4, .__asm_swi_handler_data+4\n\t"
    "ldr lr, [sl, r4]\n\t"
  "msr cpsr_c, #211\n\t"

  "msr spsr_c, #16\n\t"

  "mov lr, r1\n\t"
  "movs pc, lr\n\t"

"\n"
".__asm_swi_handler_data:\n\t"
  ".word kernelSp(GOT)\n\t"
  ".word Exit(GOT)\n\t"
  );

void __syscall(void* stack, kernel_request_t *arg) {
  // copy over request
  ctx->descriptors[arg->tid].current_request = *arg;

  // rescheduling of the world occurs in the returned assembly

  // save stack pointer
  // we need to know how to get the current task
  active_task->stack_pointer = stack;
}

void context_switch(kernel_request_t *arg) {
  // NOTE: we pass the syscall number via. arg->syscall, and arg is r0
  asm volatile ("swi #0\n\t");
}
