#include <basic.h>
#include <servers/nameserver.h>
#include <idle_task.h>
#include <ts7200.h>

void idle_task() {
  while (true) {
    // We track time in the kernel, and print it there also
    // so just loop!
    asm __volatile__("");
  }
}
