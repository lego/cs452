#include <basic.h>
#include <bwio.h>
#include <kern/context.h>
#include <kern/interrupts.h>
#include <kern/context_switch.h>
#include <kern/scheduler.h>
#include <kern/task_descriptor.h>

typedef int (*interrupt_handler)(int);
typedef void (*debugger_func)(void);

void __undefined_instruction_handler();
void __prefetch_abort_handler();
void __data_abort_handler();

void context_switch_init() {
  *((debugger_func*)0x24) = &__undefined_instruction_handler;  // undefined instr
  *((interrupt_handler*)0x28) = (interrupt_handler)&__asm_swi_handler;
  *((debugger_func*)0x2c) = &__prefetch_abort_handler;  // prefetch abort
  *((debugger_func*)0x30) = &__data_abort_handler; // data abort
  // none
  *((interrupt_handler*)0x38) = (interrupt_handler)&__asm_hwi_handler;
}
char *undefined_instr_msg = SCROLL_DOWN_20 RED_BG "UNDEFINED INSTRUCTION EXCEPTION";
char *prefetch_abort_msg = SCROLL_DOWN_20 RED_BG "PREFETCH ABORT";
char *data_abort_msg = SCROLL_DOWN_20 RED_BG "DATA ABORT";
char *kernel_fail = RESET_ATTRIBUTES "  Kernel LR: ";
char *task_fail = "   Task LR: ";

asm (
"\n"
// export of the function
".global __asm_start_task\n"
".global __asm_switch_to_task\n"


"__undefined_instruction_handler:\n\t"
  "mov r0, #1\n\t"
  "ldr r1, =undefined_instr_msg\n\t"
  "ldr r1, [r1]\n\t"
  "bl bwputstr\n\t"
  "b __abort_handler\n\t"

"__prefetch_abort_handler:\n\t"
  "mov r0, #1\n\t"
  "ldr r1, =prefetch_abort_msg\n\t"
  "ldr r1, [r1]\n\t"
  "bl bwputstr\n\t"
  "b __abort_handler\n\t"

"__data_abort_handler:\n\t"
  "mov r0, #1\n\t"
  "ldr r1, =data_abort_msg\n\t"
  "ldr r1, [r1]\n\t"
  "bl bwputstr\n\t"
  "b __abort_handler\n\t"


"__abort_handler:\n\t"
  // print kernel lr
  "msr cpsr_c, #211\n\t"
  "stmfd sp!, {lr}\n\t"
  "mov r0, #1\n\t"
  "ldr r1, =kernel_fail\n\t"
  "ldr r1, [r1]\n\t"
  "bl bwputstr\n\t"
  "ldmfd sp!, {lr}\n\t"
  "mov r0, #1\n\t"
  "mov r1, lr\n\t"
  "bl bwputr\n\t"

  // print task lr
  "mov r0, #1\n\t"
  "ldr r1, =task_fail\n\t"
  "ldr r1, [r1]\n\t"
  "bl bwputstr\n\t"

  "msr cpsr_c, #223\n\t"
  "mov r0, #1\n\t"
  "mov r1, lr\n\t"
  "msr cpsr_c, #211\n\t"
  "bl bwputr\n\t"

  "mov r0, #0\n\t"
  "b exit_kernel\n\t"

"__asm_switch_to_task:\n\t"
  // save the return-to-kernel lr + registers
  "stmfd sp!, {r4-r12, lr}\n\t"

  // in system mode
  "msr cpsr_c, #223\n\t"
    // switch to argument stack pointer
    "mov sp, r0\n\t"

    // recover task spsr and lr
    "ldmfd sp!, {r1, r2}\n\t"
  // go into svc mode, and recover the user CPSR into SPSR_svc
  "msr cpsr_c, #211\n\t"
  "msr spsr, r1\n\t"

  "mov lr, r2\n\t"

  // in system mode
  "msr cpsr_c, #223\n\t"
    // recover task registers
    "ldmfd sp!, {r0-r12, lr}\n\t"
  "msr cpsr_c, #211\n\t"

  // begin execution of task
  "movs pc, lr\n\t"

"\n"
"__asm_swi_handler:\n\t"
  // start in svc mode

  // in system mode
  "msr cpsr_c, #223\n\t"
    // store task state and lr
    "stmfd sp!, {r0-r12, lr}\n\t"
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
"__asm_hwi_handler:\n\t"
  // start in irq mode

  "sub lr, lr, #4\n\r"

  // in system mode
  "msr cpsr_c, #223\n\t"
    // store task state and lr
    "stmfd sp!, {r0-r12, lr}\n\t"
  // back to irq
  "msr cpsr_c, #210\n\t"

  "mrs r4, spsr\n\t"
  // save return-to-user lr to r5 so it's not overwritten by r0-r3
  "mov r5, lr\n\t"

  // in system mode
  "msr cpsr_c, #223\n\t"
    "stmfd sp!, {r4, r5}\n\t"
    "mov r4, sp\n\t"
  // switch to svc mode
  "msr cpsr_c, #211\n\t"

  // set up args for syscall
  "mov r0, r4\n\t" // task stack pointer

  // handle syscall
  "bl __hwint(PLT)\n\t"

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
    "ldr r4, .__asm_swi_handler_data+0\n\t"
    "ldr lr, [sl, r4]\n\t"
  "msr cpsr_c, #211\n\t"

  "msr spsr, #16\n\t"

  "mov lr, r1\n\t"
  "movs pc, lr\n\t"

"\n"
".__asm_swi_handler_data:\n\t"
  ".word Exit(GOT)\n\t"
);

void __hwint(void* stack) {
  int tid = active_task->tid;
  ctx->descriptors[tid].current_request.tid = tid;
  ctx->descriptors[tid].current_request.syscall = SYSCALL_HW_INT;

  // save stack pointer
  // we need to know how to get the current task
  active_task->stack_pointer = stack;
}

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
