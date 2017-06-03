#include <basic.h>
#include <ts7200.h>
#include <idle_task.h>
#include <nameserver.h>

void idle_task() {
  while (true) {
    // We track time in the kernel, and print it there also
    // so just loop!
    asm __volatile__("");
  }
}
