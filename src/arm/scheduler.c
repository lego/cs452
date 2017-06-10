#include <basic.h>
#include <kern/context.h>
#include <kern/context_switch.h>
#include <kern/scheduler.h>
#include <kern/task_descriptor.h>
#include <kern/arm/linker.h>

void scheduler_arch_init() {
  log_scheduler_kern("would initalize schedule");
}

void scheduler_exit_task(task_descriptor_t *task) {
  log_scheduler_task("would exit task", task->tid);
}

void scheduler_reschedule_the_world() {
  log_scheduler_task("would go back to kernel", active_task->tid);
}

void *scheduler_start_task(void *td) {
  task_descriptor_t *task = (task_descriptor_t *) td;
  log_scheduler_task("would start task", task->tid);
  return NULL;
}

kernel_request_t *scheduler_activate_task(task_descriptor_t *task) {
  // assert here to make sure the task stack pointer does not
  // extend into other task stacks
  // NOTE: casted to char * so we get the byte size count
  if ((_TaskStackStart - (char *) task->stack_pointer) > _TaskStackSize) {
    bwprintf(COM2, "WARNING: TASK STACK OVERFLOWED. tid=%d", task->tid);
    KASSERT(false, "Task stack overflowed");
  }
  log_scheduler_kern("activating task tid=%d", task->tid);
  active_task = task;
  if (!task->has_started) {
    task->has_started = true;
    __asm_start_task(task->stack_pointer, task->entrypoint);
  } else {
    __asm_switch_to_task(task->stack_pointer);
  }
  log_scheduler_kern("returned from task tid=%d", task->tid);
  active_task = NULL;
  return &task->current_request;
}
