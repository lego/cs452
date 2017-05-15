#include <kernel.h>
#include <kern/context_switch.h>

int Create(int priority, void (*code)()) {
  return context_switch(SYSCALL_CREATE, priority, code);
}

int MyTid( ) {
  return context_switch(SYSCALL_MY_TID, 0, 0);
}

int MyParentTid( ) {
  return context_switch(SYSCALL_MY_PARENT_TID, 0, 0);
}

void Pass( ) {
  context_switch(SYSCALL_PASS, 0, 0);
}

void Exit( ) {
  context_switch(SYSCALL_EXIT, 0, 0);
}
