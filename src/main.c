 /*
 * iotest.c
 */

#include <bwio.h>

typedef struct TaskState_t {
  void* sl;
  void* fp;
  void* sp;
  void* lr;
  void* pc;
} TaskState;

typedef struct TaskDescriptor_t {
  TaskState state;
} TaskDescriptor;

TaskState kernelState;

void switch_back() {
  TaskState* localState = &kernelState;
  asm(
	  "ldr	r3, [fp, #-20]\n\t"
    "ldr sl, [r3, #0]\n\t"
    "ldr fp, [r3, #4]\n\t"
    "ldr sp, [r3, #8]\n\t"
    "ldr lr, [r3, #12]\n\t"
    "ldr pc, [r3, #16]\n\t"
  );
}

int taskCode() {
  bwprintf(COM2, "taskCode: Hello, again!\n\r");
  switch_back();
  bwprintf(COM2, "taskCode: Hello, a third time!\n\r");
  return 0;
}

void switch_to(void* addr) {
  bwprintf(COM2, "switchTo: %x\n\r", addr);
  TaskState* localState = &kernelState;
  asm(
	  "ldr	r3, [fp, #-20]\n\t"
    "str sl, [r3, #0]\n\t"
    "str fp, [r3, #4]\n\t"
    "str sp, [r3, #8]\n\t"
    "str lr, [r3, #12]\n\t"
    "str pc, [r3, #16]\n\t"
  );
  asm(
    "mov lr, pc\n\t"
    "ldr pc, [fp, #-24]\n\t"
  );
  bwprintf(COM2, "switchTo: end\n\r");
  bwprintf(COM2,
      "switch_to: %x\n\r           %x\n\r           %x\n\r           %x\n\r",
      kernelState.sl,
      kernelState.fp,
      kernelState.sp,
      kernelState.pc
  );
}

int Create(int priority, void* code) {
  return 0;
}

int MyTid() {
  return 0;
}

int MyParentId() {
  return 0;
}

void Pass() {
}

void Exit() {
}

int main() {
  bwsetfifo(COM2, OFF);
  bwprintf(COM2, "main: Hello, world!\n\r");
  switch_to(&taskCode);
  bwprintf(COM2, "main: Hello, returned world!\n\r");
  return 0;
}
