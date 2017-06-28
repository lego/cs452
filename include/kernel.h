#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <io.h>

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
#define Create(priority, code) CreateWithName(priority, code, #code)
int CreateWithName(int priority, void (*code)( ), const char *name);


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

/**
 * Exit the kernel as early as possible
 */
void ExitKernel( );

#define SendS(tid, arg1, arg2) Send(tid, &arg1, sizeof(arg1), &arg2, sizeof(arg2))
#define SendSN(tid, arg1) Send(tid, &arg1, sizeof(arg1), NULL, 0)
#define ReceiveS(tid, arg1) Receive(tid, &arg1, sizeof(arg1))
#define ReplyS(tid, arg1) Reply(tid, &arg1, sizeof(arg1))

#define ReceiveN(tid) Receive(tid, NULL, 0)
#define ReplyN(tid) Reply(tid, NULL, 0)


int Send( int tid, void *msg, int msglen, volatile void *reply, int replylen);

int Receive( int *tid, volatile void *msg, int msglen );

int Reply( int tid, void *reply, int replylen );

enum await_event_t {
  EVENT_TIMER,
  EVENT_UART2_TX,
  EVENT_UART1_TX,
  EVENT_UART2_RX,
  EVENT_UART1_RX,

  // Must be last
  EVENT_NUM_TYPES,
}; typedef int await_event_t;

int AwaitEvent( await_event_t event_type );
int AwaitEventPut( await_event_t event_type, char ch );

io_time_t GetIdleTaskExecutionTime();

#endif
