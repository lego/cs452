#include <bwio.h>
#include <entry_task.h>
#include <kernel.h>
#include <ts7200.h>
#include <debug.h>

void child_task();
// void child_task() {
//   int my_tid = MyTid();
//   int my_parent_tid = MyParentTid();
//   bwprintf(COM2, "CH  MyTid=%d MyParentTid=%d\n\r", my_tid, my_parent_tid);
//   Pass();
//   bwprintf(COM2, "CH  MyTid=%d MyParentTid=%d\n\r", my_tid, my_parent_tid);
//   Exit();
// }
//
// void entry_task() {
//   bwprintf(COM2, "T0  Hello, world!\n\r");
//   int x;
//
//   bwprintf(COM2, "T0  calling pass!\n\r");
//   Pass();
//
//   bwprintf(COM2, "T0  calling MyTid!\n\r");
//   x = MyTid();
//   bwprintf(COM2, "T0  syscall ret=%d!\n\r", x);
//
//   bwprintf(COM2, "T0  calling MyParentTid!\n\r");
//   x = MyParentTid();
//   bwprintf(COM2, "T0  syscall ret=%d!\n\r", x);
//
//   bwprintf(COM2, "T0  calling Create!\n\r");
//   x = Create(PRIORITY_HIGHEST, &child_task);
//   bwprintf(COM2, "T0  syscall ret=%d!\n\r", x);
// }


void entry_task() {
  int new_task_id;
  log_debug("Task started");
  new_task_id = Create(PRIORITY_HIGHEST, &child_task);
  bwprintf(COM2, "Created: %d\n\r", new_task_id);
  new_task_id = Create(PRIORITY_HIGHEST, &child_task);
  bwprintf(COM2, "Created: %d\n\r", new_task_id);
  new_task_id = Create(PRIORITY_LOW, &child_task);
  bwprintf(COM2, "Created: %d\n\r", new_task_id);
  new_task_id = Create(PRIORITY_LOW, &child_task);
  bwprintf(COM2, "Created: %d\n\r", new_task_id);
  bwprintf(COM2, "FirstUserTask: exiting\n\r");
  Exit();
}


void child_task() {
  int my_tid = MyTid();
  int my_parent_tid = MyParentTid();
  bwprintf(COM2, "MyTid=%d MyParentTid=%d\n\r", my_tid, my_parent_tid);
  Pass();

  __asm__("msr spsr_c, #0\n\r");

  bwprintf(COM2, "MyTid=%d MyParentTid=%d\n\r", my_tid, my_parent_tid);
  // Exit();
}
