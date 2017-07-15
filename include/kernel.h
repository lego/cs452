#pragma once

#include <io.h>

// Hardcoded maximum used in a number of places
// WARNING: this is also defined in orex.ld
// if this in increased, you should also increase that one
#define MAX_TASKS 256
#define MAX_TASK_STACKS 100

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
#define Create(priority, code) _CreateWithName(priority, code, #code, false)
#define CreateRecyclable(priority, code) _CreateWithName(priority, code, #code, true)
#define CreateWithName(priority, code, name) _CreateWithName(priority, code, name, false);
int _CreateWithName(int priority, void (*code)( ), const char *name, bool is_recyclable);



const char * MyTaskName( );

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
 * Destroys a task and all of it's children
 */
void Destroy( int tid );

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
#define SendEmptyWithResp(tid, arg2) Send(tid, NULL, 0, &arg2, sizeof(arg2))
#define ReceiveS(tid, arg1) Receive(tid, &arg1, sizeof(arg1))
#define ReplyS(tid, arg1) Reply(tid, &arg1, sizeof(arg1))

#define ReceiveN(tid) Receive(tid, NULL, 0)
#define ReplyN(tid) Reply(tid, NULL, 0)

#define ReplyStatus(tid, status) do { int n = status; ReplyS(tid, n); } while(0)

int Send( int tid, void *msg, int msglen, volatile void *reply, int replylen);

int Receive( int *tid, volatile void *msg, int msglen );

int Reply( int tid, void *reply, int replylen );

void *Malloc( unsigned int size );
int Free(void * ptr);

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

void RecordLog(const char *msg);
void RecordLogi(int i);

#include <stdarg.h>
void RecordLogf(char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
