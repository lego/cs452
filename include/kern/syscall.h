#ifndef __SYSCALL_H__
#define __SYSCALL_H__

// for syscall_t definition
#include <kern/context.h>

void syscall_handle(kernel_request_t *arg);

#endif
