#include <basic.h>
#include <kern/context.h>
#include <kern/scheduler.h>
#include <kern/task_descriptor.h>

void scheduler_arch_init() {
  log_debug("SC  would initalize scheduler\n\r");
}

void scheduler_exit_task() {
  log_debug("SC  would exit task\n\r");
}

void scheduler_reschedule_the_world() {
  log_debug("SC  would go back to kernel\n\r");
}

void *scheduler_start_task(void *td) {
  task_descriptor_t *task = (task_descriptor_t *) td;
  log_debug("SC  would start task tid=%d\n\r", task->tid);
  return NULL;
}

void scheduler_activate_task(task_descriptor_t *task) {
  // FIXME: XXX SUPER IMPORTANT
  // put an assert here to make sure the task stack pointer does not
  // extend into other task stacks
  // NOTE: casted to char * so we get the byte size count
  // if ((TASK_STACK_START - (char *) task->stack_pointer) > TASK_STACK_SIZE) {
  //   bwprintf(COM2, "WARNING: TASK STACK OVERFLOWED.");
  // }
  log_debug("SC  activating task tid=%d\n\r", task->tid);
  active_task = task;
  if (!task->has_started) {
    task->has_started = true;
    __asm_start_task(task->stack_pointer, task->entrypoint);
  } else {
    __asm_switch_to_task(task->stack_pointer);
  }
  log_debug("SC  returned from task tid=%d\n\r", task->tid);
  active_task = NULL;
}
