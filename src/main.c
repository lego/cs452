 /*
 * iotest.c
 */

#include <bwio.h>

typedef int (*interrupt_handler)(int);

int swi_handler(int number) {
  bwprintf(COM2, "swi_handler: syscall %x\n\r", number & 0x0000FFFF);
  return 5;
}

void* schedule(void* stack) {
  bwprintf(COM2, "schedule: sp %x\n\r", stack);
  return stack; // restart the same task
}

void __asm_swi_handler();
asm(
"\n__asm_swi_handler:\n\t"
  "stmfd sp!, {r4-r12, lr}\n\t"
  "ldr r0, [lr, #-4]\n\t"
  // Save stack pointer of active task
  "bl swi_handler(PLT)\n\t"
  "stmfd sp!, {r0}\n\t"
  "mov r0, sp\n\t"
  "bl schedule(PLT)\n\t"
  "mov sp, r0\n\t"
  // Set stack pointer to active task
  "ldmfd sp!, {r0, r4-r12, lr}\n\t"
  "movs pc, lr\n\t"
);

int Create(int priority, void* code) {
  return 0;
}

int MyTid() {
  return 0;
}

int MyParentId() {
  return 0;
}

int Pass() {
  asm(
    "swi #3\n\t"
  );
}

void Exit() {
}

int main() {
  bwsetfifo(COM2, OFF);
  *((interrupt_handler*)0x28) = &__asm_swi_handler;
  bwprintf(COM2, "main: Hello, world!\n\r");
  int x = Pass();
  bwprintf(COM2, "main: syscall returned %d!\n\r", x);
  return 1;
}
