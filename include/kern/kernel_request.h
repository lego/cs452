#ifndef __KERNEL_REQUEST_H__
#define __KERNEL_REQUEST_H__

/*
 * kernel_request represents a syscall request to the kernel which should
 * be computed once the task goes back to execution
 */

typedef struct {
  int tid;
} kernel_request_t;

#endif
