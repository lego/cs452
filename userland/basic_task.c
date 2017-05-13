#include <kernel.h>
#include <ts7200.h>
#include <bwio.h>
#include <basic_task.h>
#include <test_task.h>

void basic_task() {
  bwputstr(COM2, "Enter: basic_task\n");
  Create(1, &test_task);
  bwputstr(COM2, "Exit: basic_task\n");
  return;
}
