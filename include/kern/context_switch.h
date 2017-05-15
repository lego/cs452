#ifndef __CONTEXT_SWITCH_H__
#define __CONTEXT_SWITCH_H__

typedef int syscall_t;
#define SYSCALL_MY_TID (syscall_t) 1
#define SYSCALL_MY_PARENT_TID (syscall_t) 2
#define SYSCALL_CREATE (syscall_t) 3
#define SYSCALL_PASS (syscall_t) 4
#define SYSCALL_EXIT (syscall_t) 5

int context_switch(syscall_t call_no, int arg1, void *arg2);

#endif
