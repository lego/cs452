#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <kern/task_descriptor.h>
#include <kern/kernel_request.h>

#define MIN_PRIORITY 0
#define MAX_PRIORITY 31

extern unsigned long int priotities_ready;

extern task_descriptor_t *ready_queues[MAX_PRIORITY + 1];
extern task_descriptor_t *ready_queues_end[MAX_PRIORITY + 1];

/*
 * functionality for scheduling and swapping the active task as
 * part of scheduling
 */

// NOTE: the below constructs refer to task switching

#ifndef SCHEDULER_INLINE
#define SCHEDULER_INLINE INLINE
#endif

/**
 * Initializes scheduler state
 * - currently global variables for x86 :(
 */
SCHEDULER_INLINE void scheduler_init();
void scheduler_arch_init();

/**
 * Exits and finishes execution of a task
 * also releases control to the kernel
 */
void scheduler_exit_task(task_descriptor_t *task);

/**
 * Yields execution of the current task, releasing control to the kernel
 */
void scheduler_reschedule_the_world();

/**
 * Entry for execution of a new task
 * NOTE: if this function completes, the task is done and so it returns
 * control to the kernel
 * @param  td the task descriptor that is now executing
 */
void *scheduler_start_task(void *td);

/**
 * Activates a task, either beginning it or resuming it. this removes control of the kernel to the task
 * This is part of swapping for the scheduler
 *
 * @param task to start executing
 */
kernel_request_t *scheduler_activate_task(task_descriptor_t *task);


// NOTE: these functions are for actual scheduling things

/**
 * Puts a task back onto the ready queue
 */
void scheduler_requeue_task(task_descriptor_t *task);

/**
 * Checks if there are any other tasks to schedule
 */
#define scheduler_any_task() priotities_ready
/**
 * Puts a task back onto the ready queue
 */
SCHEDULER_INLINE task_descriptor_t *scheduler_next_task();

/**
 * Gets the count of tasks currently on the ready queue
 * NOTE: should only be used for debugging
 */
int scheduler_ready_queue_size();


#endif
