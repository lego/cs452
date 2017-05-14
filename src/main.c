/*
 * iotest.c
 */

#include <bwio.h>
#include <basic.h>
#include <io_util.h>
#include <basic_task.h>
#include <kernel.h>
#include <kern/task_descriptor.h>

typedef struct {
  int tid;
} KernelRequest;

static TaskDescriptor tasks[5];
static unsigned int task_count;

void initialize() {
  // initialize kernel logic
  // create first user task
  TaskDescriptor task = {
    .tid = 1,
    .entrypoint = &basic_task
  };
  tasks[task_count++] = task;
}

int Create(int priority, void (*code)()) {
  TaskDescriptor task = {
    .tid = 1,
    .entrypoint = code
  };
  tasks[task_count++] = task;
  return 0;
}

TaskDescriptor schedule() {
  TaskDescriptor next_task = tasks[0];
  unsigned int i;
  for (i = 0; i < 4; i++) {
    tasks[i] = tasks[i+1];
  }
  task_count--;
  return next_task;
}

KernelRequest *activate(TaskDescriptor task) {
  task.entrypoint();
  KernelRequest *kr = NULL;
  return kr;
}

void handle(KernelRequest *request) {
  return;
}

int main() {
  ts7200_init();
  initialize();
  while (task_count > 0) {
    TaskDescriptor next_task = schedule();
    KernelRequest *request = activate(next_task);
    handle(request);
  }
  return 0;
}
