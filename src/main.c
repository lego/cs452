 /*
 * iotest.c
 */

asm(
  "interrupt_vector_table:\n\t"
  "  b . @ Reset Handler\n\t"
  "  b . @ Undefined\n\t"
  "  b swi_handler @ SWI Handler\n\t"
  "  b . @ Prefetch Abort\n\t"
  "  b . @ Data Abort\n\t"
  "  b . @ IRQ\n\t"
  "  b . @ FIQ\n\t"
);

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

void swi_handler(unsigned int number) {
  bwprintf(COM2, "C_SWI_handler: syscall %d\n\r", number);
  asm(
    "movs pc, lr\n\t"
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
  asm(
    "swi #3"
  );
}

void Exit() {
}

int main() {
  bwsetfifo(COM2, OFF);
  //*((int*)0x8) = &swi_handler;
  //*((int*)0x20) = &swi_handler;
  //*((int*)0x28) = &swi_handler;
  bwprintf(COM2, "main: Hello, world!\n\r");
  // TODO: This code moves the pc to swi_handler from anywhere.
  //   Need to put that code at 0x28? Or at least somewhere reachable from 0x08
  //   Should check what instructions are at 0x08 by default
  //   If it's mov pc, [pc, #18], check what's at 0x08 + #18
  asm(
    "ldr r3, [pc, #8]\n\t"
    "ldr	r3, [sl, r3]\n\t"
    "mov lr, pc\n\t"
    "mov pc, r3\n\t"
    ".word	swi_handler(GOT)\n\t"
  );
  bwprintf(COM2, "main: asdf!\n\r");
  //asm(
  //  "mov lr, pc\n\t"
  //  "mov pc, #8\n\t"
  //);
  //asm(
  //  "mov r3, #8\n\t"
  //  "ldr	r3, [r3, #0]\n\t"
  //  "mov lr, pc\n\t"
  //  "mov pc, r3\n\t"
  //);
  //asm(
  //  "swi #3"
  //);
  //Pass();
  switch_to(&taskCode);
  bwprintf(COM2, "main: Hello, returned world!\n\r");
  return 0;
}
asm(
  "\n.main_data:\n\t"
  "ldr r3, [pc, #8]\n\t"
  "ldr	r3, [sl, r3]\n\t"
  "mov lr, pc\n\t"
  "mov pc, r3\n\t"
  ".word	swi_handler(GOT)\n\t"
);
