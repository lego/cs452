#include <kernel.h>
#include <basic_task.h>
#include <test_task.h>

void basic_task() {
  Create(1, test_task);
  return;
}
