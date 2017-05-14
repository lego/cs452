/*
 * iotest.c
 */

#define TASKDESCRIPTORINLINE

#include <basic.h>
#include <bwio.h>
#include <cbuffer.h>
#include <io_util.h>
#include <basic_task.h>
#include <kernel.h>
#include <alloc.h>
#include <kern/task_descriptor.h>
#include <kern/kernel_request.h>

static cbuffer tasks_arr;

void initialize() {
  void *tasks_buf[100];
  tasks_arr = cbuffer_create(tasks_buf, 100);

  // initialize kernel logic
  // create first user task
  task_descriptor *task2 = alloc(sizeof(task_descriptor));
  task2->tid = 1;
  task2->entrypoint = &basic_task;
  cbuffer_add(&tasks_arr, task2);
}

int Create(int priority, void (*code)()) {
  task_descriptor *task2 = alloc(sizeof(task_descriptor));
  task2->tid = 1;
  task2->entrypoint = code;
  cbuffer_add(&tasks_arr, task2);

  return 0;
}

task_descriptor schedule() {
  int status;
  task_descriptor *next_task = cbuffer_pop(&tasks_arr, &status);
  debugger();
  return *next_task;
}

KernelRequest *activate(task_descriptor task) {
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
  while (!cbuffer_empty(&tasks_arr)) {
    task_descriptor next_task = schedule();
    KernelRequest *request = activate(next_task);
    handle(request);
  }
  return 0;
}
