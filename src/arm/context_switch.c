#include <basic.h>
#include <bwio.h>
#include <kern/arm/swi_handler.h>
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

  // save kernel state and lr
  "stmfd sp!, {r4-r12, lr}\n\t"

  // save kernel sp
  "ldr r4, .__asm_swi_handler_data+0\n\t"
  "str sp, [sl, r4]\n\t"

  // switch to argument stack pointer
  "movs sp, r0\n\t"

  // recover task registers
  "ldmfd sp!, {r1, r4-r12, lr}\n\t"

  // begin execution of task
  "movs pc, lr\n\t"

"\n"
"__asm_swi_handler:\n\t"
  // store task state and lr
  "stmfd sp!, {r4-r12, lr}\n\t"

  // "mrs r4, spsr\n\t"
  "stmfd sp!, {r4}\n\t"

  // set up args for syscall
  "mov r3, r2\n\t" // sycall arg2
  "mov r2, r1\n\t" // sycall arg1
  "mov r1, r0\n\t" // syscall number
  "mov r0, sp\n\t" // task stack pointer

  // switch to kernel stack
  "ldr r4, .__asm_swi_handler_data+0\n\t"
  "ldr sp, [sl, r4]\n\t"

  // handle syscall
  "bl __syscall(PLT)\n\t"

  // switch to kernel stack before the syscall
  "ldr r4, .__asm_swi_handler_data+0\n\t"
  "ldr sp, [sl, r4]\n\t"

  // load kernel state before tasks were finished
  "ldmfd sp!, {r4-r12, lr}\n\t"
  // return from scheduler_activate_task in kernel
  "mov pc, lr\n\t"

"\n"
"__asm_start_task:\n\t"
  // save kernel state and lr
  "stmfd sp!, {r4-r12, lr}\n\t"

  // save kernel sp
  "ldr r4, .__asm_swi_handler_data+0\n\t"
  "str sp, [sl, r4]\n\t"

  // switch to new stack
  "mov sp, r0\n\t"

  // Store Exit function as the return place
  "ldr r4, .__asm_swi_handler_data+4\n\t"
  "ldr lr, [sl, r4]\n\t"

  // "mrs r0, spsr\n\t" // load in spsr
  // "bic r0, r0, #31\n\t" // mask out mode
  // "orr r0, r0, #16\n\t" // set user mode
  // "msr spsr_c, r0\n\t" // set spsr
  //
  // "mov r0, #1\n\t"
  // "mrs r1, cpsr\n\t"
  // "bl bwputr\n\t"

  // "mrs r1, spsr\n\t"
  // "bl bwputr\n\t"

  "movs pc, r1\n\t"

"\n"
".__asm_swi_handler_data:\n\t"
  ".word kernelSp(GOT)\n\t"
  ".word Exit(GOT)\n\t"
  );

void __syscall(void* stack, kernel_request_t *arg) {
  // copy over request
  ctx->descriptors[arg->tid].current_request = *arg;

  syscall_handle(&(ctx->descriptors[arg->tid].current_request));

  // rescheduling of the world occurs in the returned assembly

  // save stack pointer
  // we need to know how to get the current task
  active_task->stack_pointer = stack;
}

void context_switch(kernel_request_t *arg) {
  // NOTE: we pass the syscall number via. r0 as it's the first argument
  asm ("swi #0\n\t");
}
