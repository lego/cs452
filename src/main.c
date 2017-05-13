/*
 * iotest.c
 */

#include <bwio.h>
#include <basic.h>
#include <io_util.h>


typedef struct {
  int tid;
} TaskDescriptor;

typedef struct {
  int tid;
} KernelRequest;

void initialize() {
  // initialize kernel logic
  // create first user task
}

TaskDescriptor *schedule() {
  TaskDescriptor *td = NULL;
  return td;
}

KernelRequest *activate(TaskDescriptor *task) {
  KernelRequest *kr = NULL;
  return kr;
}

void handle(KernelRequest *request) {
  return;
}

int main() {
  ts7200_init();
  bwputstr(COM2, "hello");
  initialize();
  while (true) {
    TaskDescriptor *next_task = schedule();
    KernelRequest *request = activate(next_task);
    handle(request);
  }
  return 0;
}
