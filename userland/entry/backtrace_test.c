#include <basic.h>
#include <bwio.h>
#include <entry/backtrace_test.h>
#include <kernel.h>


// void hex_dump(char *desc, void *addr, int len)  {
//     int i;
//     unsigned char buff[17];
//     unsigned char *pc = (unsigned char*)addr;
//
//     // Output description if given.
//     if (desc != NULL)
//         printf ("%s:\n", desc);
//
//     // Process every byte in the data.
//     for (i = 0; i < len; i++) {
//         // Multiple of 16 means new line (with line offset).
//
//         if ((i % 16) == 0) {
//             // Just don't print ASCII for the zeroth line.
//             if (i != 0)
//                 printf("  %s\n", buff);
//
//             // Output the offset.
//             printf("  %04x ", i);
//         }
//
//         // Now the hex code for the specific character.
//         printf(" %02x", pc[i]);
//
//         // And store a printable ASCII character for later.
//         if ((pc[i] < 0x20) || (pc[i] > 0x7e)) {
//             buff[i % 16] = '.';
//         } else {
//             buff[i % 16] = pc[i];
//         }
//
//         buff[(i % 16) + 1] = '\0';
//     }
//
//     // Pad out last line if not exactly 16 characters.
//     while ((i % 16) != 0) {
//         printf("   ");
//         i++;
//     }
//
//     // And print the final ASCII bit.
//     printf("  %s\n", buff);
// }

// static inline void print_registers() {
//   bwprintf(COM2, "\tfp=");
//   asm volatile(
//     "mov r0, #1 @ COM2\n\t"
//     "mov r1, fp @ printing fp \n\t"
//     "bl bwputr \n\t"
//   );
//   // bwprintf(COM2, "\n\r\tip=");
//   // asm volatile(
//   //   "mov r0, #1 @ COM2\n\t"
//   //   "mov r1, ip @ printing ip \n\t"
//   //   "bl bwputr \n\t"
//   // );
//   // bwprintf(COM2, "\n\r\tpc=");
//   // asm volatile(
//   //   "mov r0, #1 @ COM2\n\t"
//   //   "mov r1, pc @ printing pc \n\t"
//   //   "bl bwputr \n\t"
//   // );
//   bwprintf(COM2, "\n\r\tlr=");
//   asm volatile(
//     "mov r0, #1 @ COM2\n\t"
//     "mov r1, lr @ printing lr \n\t"
//     "bl bwputr \n\t"
//   );
//   // bwprintf(COM2, "\n\r\tr0=");
//   // asm volatile(
//   //   "mov r0, #1 @ COM2\n\t"
//   //   "mov r1, r0 @ printing r0 \n\t"
//   //   "bl bwputr \n\t"
//   // );
//   // bwprintf(COM2, "\n\r\tr1=");
//   // asm volatile(
//   //   "mov r0, #1 @ COM2\n\t"
//   //   "mov r1, r1 @ printing r1 \n\t"
//   //   "bl bwputr \n\t"
//   // );
//   // bwprintf(COM2, "\n\r\tr2=");
//   // asm volatile(
//   //   "mov r0, #1 @ COM2\n\t"
//   //   "mov r1, r2 @ printing r2 \n\t"
//   //   "bl bwputr \n\t"
//   // );
//   // bwprintf(COM2, "\n\r\tr3=");
//   // asm volatile(
//   //   "mov r0, #1 @ COM2\n\t"
//   //   "mov r1, r3 @ printing r3 \n\t"
//   //   "bl bwputr \n\t"
//   // );
//   // bwprintf(COM2, "\n\r\tr4=");
//   // asm volatile(
//   //   "mov r0, #1 @ COM2\n\t"
//   //   "mov r1, r4 @ printing r4 \n\t"
//   //   "bl bwputr \n\t"
//   // );
//   bwprintf(COM2, "\n\r");
// }

// char *get_func_name(unsigned int *fp) {
//   unsigned int name_length = *(fp - 2) >> 16;
//   char *chars = (char *) *fp;
//   chars -= name_length + 3;
//   // bwprintf(COM2, "Function name length=%d name=%s\n\r", name_length, chars);
//   return chars;
// }


// void print_memory(unsigned int fp, int size) {
//   bwprintf(COM2, "region is %x\n\r", fp);
//   int i;
//   int j;
//   for (i = (4 * size) - 4; i >= 0; i -= 4) {
//     bwprintf(COM2, "%x is %x  ", fp - i, VMEM(fp - i));
//     for (j = 1; j <= 4; j++) {
//       char c = CMEM(fp - i - 4 + j);
//       if (is_alphanumeric(c) || c == '_') bwputc(COM2, c);
//       else bwputc(COM2, '.');
//     }
//     bwputstr(COM2, "\n\r");
//   }
// }

#ifndef DEBUG_MODE
void backtrace_callee() {
  bwprintf(COM2, "In backtrace_callee\n\r");
  // print_registers();
  unsigned int *fp;
  asm volatile("mov %0, fp @ save fp" : "=r" (fp));

  // int i;
  // for (i = 0; i < 30; i++) {
  //   bwprintf(COM2, "%x is %x\n\r", fp - i, fp[-i]);
  // }

  // get_func_name(fp);
  unsigned long int deadbeef = 0xdeadbeef;
  asm volatile("mov r5, %0" : : "r" (deadbeef) : "r5");
  print_stack_trace((unsigned int) fp, 0);

  // asm volatile("mov r0, #1 @ COM2" : : : "r0");
  // asm volatile("mov r1, fp @ fp + 0 => pc" : : : "r1");
  // asm volatile("bl bwputr @ print the function location");

  // bwputc(COM2, '\n');
  // bwputc(COM2, '\r');

  // asm volatile("bl bwputr @ print the function");
  // asm volatile("ldrb r1, [fp, #12] @ get top 8 bits for character check" : : : "r1");
  // asm volatile("bl bwputr @ print the length of the function name");

  // bwputc(COM2, '\n');
  // bwputc(COM2, '\r');

  // asm volatile("sub r1, fp, r1 @ PC - strlen" : : : "r1");
  // asm volatile("sub r1, r1, #12 @ PC - strlen - 12 => start of string" : : : "r1");
  // asm volatile("bl bwputstr @ print the function string");
}


void other_task() {
  int parent = MyParentTid();
  bwprintf(COM2, "Printing stack for parent task\n\r");
  PrintTaskBacktrace(parent);
  KASSERT(false, "Forceful fail");
  ExitKernel();
}

void second_call() {
  unsigned int fp; asm volatile("mov %0, fp @ save fp" : "=r" (fp));

  bwprintf(COM2, "In second_call. fp=%08x\n\r", fp);
  // print_registers();
  Create(0, other_task);

  backtrace_callee();
}

void first_call() {
  bwprintf(COM2, "In first_call\n\r");
  // print_registers();

  second_call();
}


void backtrace_test() {
  bwprintf(COM2, "In backtrace_test\n\r");
  // print_registers();

  first_call();
  ExitKernel();
}

#else
void backtrace_test() {
}
#endif
