#pragma once

// for syscall_t definition
#include <kern/context.h>

void syscall_handle(kernel_request_t *arg);
