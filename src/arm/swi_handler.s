#include <kern/arm/swi_handler.h>
#include <kernel.h>

__asm_swi_handler:
  stmfd sp!, {r4-r12, lr}

  # set up args for syscall
  mov r2, r1
  mov r1, r0
  mov r0, sp
  ldr r1, [lr, #-4]

  # switch to kernel stack
  ldr r4, .__asm_swi_handler_data+0
  ldr sp, [sl, r4]

  # handle syscall
  bl __syscall(PLT)

  # go to __asm_finished_tasks if not given a stack pointer
  cmp r0, #0
  beq __asm_finished_tasks

  # save kernel sp
  ldr r4, .__asm_swi_handler_data+0
  str sp, [sl, r4]

  # switch to returned stack pointer
  mov sp, r0

  # restore registers, including an extra r0 return value added by __syscall
  ldmfd sp!, {r0, r4-r12, lr}
  movs pc, lr


__asm_finished_tasks:
  # switch to kernel stack before the syscall
  ldr r4, .__asm_swi_handler_data+0
  ldr sp, [sl, r4]

  # load kernel state before tasks were finished
  ldmfd sp!, {r4-r12, lr}
  # continue
  mov pc, lr


__asm_start_task:
  # save kernel lr
  stmfd sp!, {r4-r12, lr}

  # save kernel sp
  ldr r4, .__asm_swi_handler_data+0
  str sp, [sl, r4]

  # switch to new stack
  mov sp, r0

  # Store Exit function as the return place
  ldr r4, .__asm_swi_handler_data+4
  ldr lr, [sl, r4]

  movs pc, r1

.__asm_swi_handler_data:
  .word kernel_stack_pointer(GOT)
  .word Exit(GOT)
  .word __syscall(GOT)
