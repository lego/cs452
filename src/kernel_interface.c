#include <basic.h>
#include <kern/context_switch.h>
#include <kernel.h>

int Create(int priority, void (*code)()) {
  void *v = context_switch(SYSCALL_CREATE, priority, code);
  log_debug("Tx  cpsr[4:0]=%d\n\r", (long long) v & 255);
  return (int) v;
}

int MyTid( ) {
  void *v =context_switch(SYSCALL_MY_TID, 0, NULL);
  log_debug("Tx  cpsr[4:0]=%d\n\r", (long long) v & 255);
  return 1;
}

int MyParentTid( ) {
  return (int) context_switch(SYSCALL_MY_PARENT_TID, 0, NULL);
}

void Pass( ) {
  void *v = context_switch(SYSCALL_PASS, 0, NULL);
  log_debug("Tx  cpsr[4:0]=%d\n\r", (long long) v & 255);
}

void Exit( ) {
  context_switch(SYSCALL_EXIT, 0, NULL);
}
