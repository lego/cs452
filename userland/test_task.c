#include <test_task.h>
#include <ts7200.h>
#include <bwio.h>

void test_task() {
  bwputstr(COM2, "Enter: test_task\n");
  bwputstr(COM2, "Exit: test_task\n");
  return;
}
