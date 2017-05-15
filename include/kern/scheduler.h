#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <kern/task_descriptor.h>

void scheduler_init();

void scheduler_exit_task();
void scheduler_reschedule_the_world();
void *scheduler_start_task(void *arg);
void scheduler_activate_task(task_descriptor *task);

#endif
