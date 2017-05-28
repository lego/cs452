#ifndef __KERNEL_H__
#define __KERNEL_H__

// Hardcoded maximum used in a number of places
// WARNING: this is also defined in orex.ld
// if this in increased, you should also increase that one
#define MAX_TASKS 100

/*
 * Kernel system calls
 * All of these functions are part of the kernels interface
 * which cause a context switch and rescheduling
 */

// TODO: if possible these should be inline

/**
 * Creates a task
 * @param  priority
 * @param  code
 * @return          the new tasks ID
 */
int Create(int priority, void (*code)( ) );

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


int Send( int tid, void *msg, int msglen, void *reply, int replylen);

int Receive( int *tid, void *msg, int msglen );

int Reply( int tid, void *reply, int replylen );

#endif
