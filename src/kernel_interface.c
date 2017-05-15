#include <basic.h>
#include <kern/context_switch.h>
#include <kernel.h>

int Create(int priority, void (*code)()) {
  // asm ("swi #3\n\t");
  // return context_switch(SYSCALL_CREATE, priority, code);
  asm ("swi #3\n\t");
}

int MyTid( ) {
  // return context_switch(SYSCALL_MY_TID, 0, NULL);
  asm ("swi #1\n\t");

}

int MyParentTid( ) {
  // return context_switch(SYSCALL_MY_PARENT_TID, 0, NULL);
  asm ("swi #2\n\t");
}

void Pass( ) {
  asm ("swi #4\n\t");
  // context_switch(SYSCALL_PASS, 0, NULL);
}

void Exit( ) {
  // context_switch(SYSCALL_EXIT, 0, NULL);
  asm ("swi #5\n\t");
}
