#ifndef __KERNEL_H__
#define __KERNEL_H__

/*
 * Kernel system calls
 * All of these functions are part of the kernels interface
 * which cause a context switch and rescheduling
 */

// TODO: if possible these should be inline

typedef int task_priority_t;
#define PRIORITY_HIGHEST (task_priority_t) 0
#define PRIORITY_MEDIUM (task_priority_t) 1
#define PRIORITY_LOW (task_priority_t) 2
#define PRIORITY_LOWEST (task_priority_t) 3

/**
 * Creates a task
 * @param  priority
 * @param  code
 * @return          the new tasks ID
 */
int Create(task_priority_t priority, void (*code)( ) );

/**
 * Gets the current task ID
 * @return a task ID
 */
int MyTid( );

/**
 * Gets the parent task ID
 * @return a task ID
 */
int MyParentTid( );

/**
 * Yield the current tasks execution, possibly causing the task to get rescheduled
 */
void Pass( );

/**
 * Exit the task, ending execution
 */
void Exit( );

#endif
