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

int context_switch_in(syscall_t call_no, int arg1, void *arg2);
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
    "ldmfd sp!, {r0, r1, r2, r4-r12, lr}\n\t"
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
  "mov r3, r2\n\t" // sycall arg2
  "mov r2, r1\n\t" // sycall arg1
  "mov r1, r0\n\t" // syscall number
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

  // FIXME: set task spsr to user mode
  "msr spsr_c, #16\n\t"

  "mov lr, r1\n\t"
  "movs pc, lr\n\t"

"\n"
".__asm_swi_handler_data:\n\t"
  ".word kernelSp(GOT)\n\t"
  ".word Exit(GOT)\n\t"
  );

void* __syscall(void* stack, syscall_t syscall_no, int arg1, void *arg2) {

  // do the syscall
  int syscall_return = context_switch_in(syscall_no, arg1, arg2);
  log_context_switch("syscall ret=%d", syscall_return);

  // Put the syscall return value on the passed in stack
  // note: this has to happen first before saving the stack pointer, as this is the return value
  asm (
    "stmfd %1!, {%0}\n\t"
    : /* no output */
    : "r" (syscall_return), "r" (stack)
    );
  stack -= 4;

  // determine which task to run next
  // task_descriptor_t *next_task = scheduler_next_task();
  // // save stack pointer
  // // we need to know how to get the current task
  active_task->stack_pointer = stack;
  // void* taskToRunStack = next_task->stack_pointer; // For now just run the same task

  return 0;
}

int context_switch(syscall_t call_no, int arg1, void *arg2) {
  // NOTE: we pass the syscall number via. r0 as it's the first argument
  asm ("swi #0\n\t");
}

int context_switch_in(syscall_t call_no, int arg1, void *arg2) {
  int ret_val = 0;

  switch (call_no) {
  case SYSCALL_MY_TID:
    log_context_switch("syscall=MyTid ret=%d", active_task->tid);
    ret_val = active_task->tid;
    scheduler_requeue_task(active_task);
    break;
  case SYSCALL_MY_PARENT_TID:
    log_context_switch("syscall=MyParentTid ret=%d", active_task->parent_tid);
    ret_val = active_task->parent_tid;
    scheduler_requeue_task(active_task);
    break;
  case SYSCALL_CREATE:
    log_context_switch("syscall=Create");
    task_descriptor_t *new_task = td_create(ctx, active_task->tid, arg1, arg2);
    log_context_switch("new task priority=%d tid=%d", arg1, new_task->tid);
    scheduler_requeue_task(new_task);
    scheduler_requeue_task(active_task);
    ret_val = new_task->tid;
    break;
  case SYSCALL_PASS:
    log_context_switch("syscall=Pass");
    scheduler_requeue_task(active_task);
    break;
  case SYSCALL_EXIT:
    log_context_switch("syscall=Exit");
    scheduler_exit_task();
    break;
  default:
    break;
  }

  log_context_switch("queue size=%d", scheduler_ready_queue_size());

  // Reschedule the world
  // scheduler_reschedule_the_world();

  return ret_val;
}
