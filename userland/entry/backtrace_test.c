#include <basic.h>
#include <bwio.h>
#include <kernel.h>


#ifndef DEBUG_MODE
void backtrace_callee() {
  bwprintf(COM2, "In backtrace_callee\n\r");

  unsigned int *fp;
  asm volatile("mov %0, fp @ save fp" : "=r" (fp));

  unsigned long int deadbeef = 0xdeadbeef;
  asm volatile("mov r5, %0" : : "r" (deadbeef) : "r5");
  print_stack_trace((unsigned int) fp, 0);

}


void other_task() {
  int my_tid = MyTid();
  int parent = MyParentTid();
  bwprintf(COM2, "other_task tid=%d\n\r", my_tid);
  Create(1, other_task);
  if (my_tid == 50) ExitKernel();
  // bwprintf(COM2, "Printing stack for parent task\n\r");
  // PrintTaskBacktrace(parent);
  // KASSERT(false, "Forceful fail");
}

void second_call() {
  unsigned int fp; asm volatile("mov %0, fp @ save fp" : "=r" (fp));

  bwprintf(COM2, "In second_call. fp=%08x\n\r", fp);
  Create(0, other_task);

  backtrace_callee();
}

void first_call() {
  bwprintf(COM2, "In first_call\n\r");

  second_call();
}


void backtrace_test() {
  bwprintf(COM2, "In backtrace_test\n\r");

  first_call();
}

#else
void backtrace_test() {
}
#endif
