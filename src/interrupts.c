#include <basic.h>
#include <kern/interrupts.h>

// the current waiting task for various events
// right now we only have event = 0, EVENT_TIMER
#define EVENT_TYPES 1
task_descriptor_t *waiting_tasks[EVENT_TYPES];

int waiting_task_count;

void interrupts_init() {
  waiting_task_count = 0;
  int i;
  for (i = 0; i < EVENT_TYPES; i++) {
    waiting_tasks[i] = NULL;
  }

  interrupts_arch_init();
}


void interrupts_set_waiting_task(await_event_t event_type, task_descriptor_t *task) {
  assert(0 <= event_type && event_type < EVENT_TYPES);
  waiting_tasks[event_type] = task;
}

void interrupts_clear_waiting_task(await_event_t event_type) {
  assert(0 <= event_type && event_type < EVENT_TYPES);
  waiting_tasks[event_type] = NULL;
}

task_descriptor_t *interrupts_get_waiting_task(await_event_t event_type) {
  assert(0 <= event_type && event_type < EVENT_TYPES);
  return waiting_tasks[event_type];
}
