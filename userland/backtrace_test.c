#include <basic.h>
#include <bwio.h>
#include <backtrace_test.h>
#include <kernel.h>

#define VMEM(x) (*(unsigned int volatile * volatile)(x))
#define CMEM(x) (*(char volatile * volatile)(x))

static inline print_registers() {
  bwprintf(COM2, "\tfp=");
  asm volatile(
    "mov r0, #1 @ COM2\n\t"
    "mov r1, fp @ printing fp \n\t"
    "bl bwputr \n\t"
  );
  // bwprintf(COM2, "\n\r\tip=");
  // asm volatile(
  //   "mov r0, #1 @ COM2\n\t"
  //   "mov r1, ip @ printing ip \n\t"
  //   "bl bwputr \n\t"
  // );
  // bwprintf(COM2, "\n\r\tpc=");
  // asm volatile(
  //   "mov r0, #1 @ COM2\n\t"
  //   "mov r1, pc @ printing pc \n\t"
  //   "bl bwputr \n\t"
  // );
  bwprintf(COM2, "\n\r\tlr=");
  asm volatile(
    "mov r0, #1 @ COM2\n\t"
    "mov r1, lr @ printing lr \n\t"
    "bl bwputr \n\t"
  );
  // bwprintf(COM2, "\n\r\tr0=");
  // asm volatile(
  //   "mov r0, #1 @ COM2\n\t"
  //   "mov r1, r0 @ printing r0 \n\t"
  //   "bl bwputr \n\t"
  // );
  // bwprintf(COM2, "\n\r\tr1=");
  // asm volatile(
  //   "mov r0, #1 @ COM2\n\t"
  //   "mov r1, r1 @ printing r1 \n\t"
  //   "bl bwputr \n\t"
  // );
  // bwprintf(COM2, "\n\r\tr2=");
  // asm volatile(
  //   "mov r0, #1 @ COM2\n\t"
  //   "mov r1, r2 @ printing r2 \n\t"
  //   "bl bwputr \n\t"
  // );
  // bwprintf(COM2, "\n\r\tr3=");
  // asm volatile(
  //   "mov r0, #1 @ COM2\n\t"
  //   "mov r1, r3 @ printing r3 \n\t"
  //   "bl bwputr \n\t"
  // );
  // bwprintf(COM2, "\n\r\tr4=");
  // asm volatile(
  //   "mov r0, #1 @ COM2\n\t"
  //   "mov r1, r4 @ printing r4 \n\t"
  //   "bl bwputr \n\t"
  // );
  bwprintf(COM2, "\n\r");
}

// char *get_func_name(unsigned int *fp) {
//   unsigned int name_length = *(fp - 2) >> 16;
//   char *chars = (char *) *fp;
//   chars -= name_length + 3;
//   // bwprintf(COM2, "Function name length=%d name=%s\n\r", name_length, chars);
//   return chars;
// }

char *get_func_name(unsigned int *pc) {
  unsigned int name_length = *(pc - 2) >> 16;
  char *chars = (char *) *pc;
  chars -= name_length + 3;
  // bwprintf(COM2, "Function name length=%d name=%s\n\r", name_length, chars);
  return chars;
}

void print_memory(unsigned int fp, int size) {
  bwprintf(COM2, "region is %x\n\r", fp);
  int i;
  for (i = 4*size; i >= 0; i-=8) {
    bwprintf(COM2, "%x is %x %x   ", fp - i, VMEM(fp - i), VMEM(fp - i - 4));
    bwprintf(COM2, "%c.%c.%c.%c.%c.%c.%c.%c\n\r", CMEM(fp - i), CMEM(fp - i - 1), CMEM(fp - i - 2), CMEM(fp - i - 3), CMEM(fp - i - 4), CMEM(fp - i - 5), CMEM(fp - i - 6), CMEM(fp - i - 7));
    // bwprintf(COM2, "%c.%c.%c.%c.%c.%c.%c.%c\n\r", CMEM(fp - i -7), CMEM(fp - i - 6), CMEM(fp - i - 5), CMEM(fp - i - 4), CMEM(fp - i - 3), CMEM(fp - i - 2), CMEM(fp - i - 1), CMEM(fp - i));
  }
}


void print_stack_trace(unsigned int fp) {
  unsigned long int deadbeef = 0xaabbccee;
  asm volatile("mov r5, %0" : : "r" (deadbeef) : "r5");
  print_registers();

	if (!fp) return;
	int pc = 0, lr = 0, depth = 20;

	if (fp <= 0x218000 || fp >= 0x2000000) {
		bwprintf(1, "fp out of range: %x\n", fp);
		return;
	}

	do {
    print_memory(fp, 16);

		pc = VMEM(fp) - 16;

    print_memory(pc, 30);

		int asm_line_num = (lr == 0) ? 0 : ((lr - pc) >> 2);
		// if (one) {
		// 	bwprintf(1, "%d\t\t%x\t%s\n", asm_line_num, pc, find_function_name(pc));
		// } else {
      // bwprintf(1, "%s @ %x+%d, ", find_function_name(pc), pc, asm_line_num);
      // char *name = get_func_name((unsigned int *)pc);

      unsigned int name_length = *(((unsigned int *)pc) - 2) >> 16;
      char *chars = (char *) (pc - 2);
      chars -= name_length + 3;
      // bwprintf(COM2, "Function name length=%d name=%s\n\r", name_length, chars);

      bwprintf(1, "====> name location=%x pc=%x len=%d\n\r", chars, pc, name_length);
      bwprintf(1, "====> char_pre=%x\n\r", (pc - 2));

      bwprintf(1, "====> %s @ %x+%d, \n\r", chars, pc, asm_line_num);
		// }

		if (lr == (int) Exit) break;

		lr = VMEM(fp - 4);
		fp = VMEM(fp - 12);
    break;

		/* if (fp < (int) &_KERNEL_MEM_START || (int) &_KERNEL_MEM_END <= fp) {
			break;
		} else */ if (depth-- < 0) {
			break;
		}
	} while (pc != REDBOOT_ENTRYPOINT);
  // } while (pc != REDBOOT_ENTRYPOINT && pc != (int) main);
}


#ifndef DEBUG_MODE
volatile void backtrace_callee() {
  bwprintf(COM2, "In backtrace_callee\n\r");
  print_registers();
  unsigned int *fp;
  asm volatile("mov %0, fp @ save fp" : "=r" (fp));

  // int i;
  // for (i = 0; i < 30; i++) {
  //   bwprintf(COM2, "%x is %x\n\r", fp - i, fp[-i]);
  // }

  // get_func_name(fp);
  unsigned long int deadbeef = 0xdeadbeef;
  asm volatile("mov r5, %0" : : "r" (deadbeef) : "r5");
  print_stack_trace(fp);

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

void second_call() {
  bwprintf(COM2, "In second_call\n\r");
  print_registers();

  backtrace_callee();
}

void first_call() {
  bwprintf(COM2, "In first_call\n\r");
  print_registers();

  second_call();
}


void backtrace_test() {
  bwprintf(COM2, "In backtrace_test\n\r");
  print_registers();

  first_call();
}

#else
void backtrace_test() {
}
#endif
