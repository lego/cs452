#include <basic.h>
#include <bwio.h>
#include <entry_task.h>
#include <kernel.h>
#include <ts7200.h>

void child_task() {
  int my_tid = MyTid();
  int my_parent_tid = MyParentTid();
  bwprintf(COM2, "MyTid=%d MyParentTid=%d\n\r", my_tid, my_parent_tid);
  Pass();
  bwprintf(COM2, "MyTid=%d MyParentTid=%d\n\r", my_tid, my_parent_tid);
  Exit();
}

void k1_entry_task() {
  int new_task_id;
  new_task_id = Create(PRIORITY_LOW, &child_task);
  bwprintf(COM2, "Created: %d\n\r", new_task_id);
  new_task_id = Create(PRIORITY_LOW, &child_task);
  bwprintf(COM2, "Created: %d\n\r", new_task_id);
  new_task_id = Create(PRIORITY_HIGHEST, &child_task);
  bwprintf(COM2, "Created: %d\n\r", new_task_id);
  new_task_id = Create(PRIORITY_HIGHEST, &child_task);
  bwprintf(COM2, "Created: %d\n\r", new_task_id);
  bwprintf(COM2, "FirstUserTask: exiting\n\r");
  Exit();
}
