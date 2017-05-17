#include <basic.h>
#include <kern/context.h>
#include <kern/scheduler.h>
#include <kern/task_descriptor.h>
#include <pthread.h>

pthread_cond_t task_cvs[MAX_TASKS];
bool task_started[MAX_TASKS];
pthread_t tasks[MAX_TASKS];
pthread_cond_t kernel_cv;
pthread_mutex_t active_mutex;

void scheduler_arch_init() {
  int i;
  for (i = 0; i < MAX_TASKS; i++) {
    pthread_cond_init(&task_cvs[i], NULL);
    task_started[i] = false;
  }
  pthread_cond_init(&kernel_cv, NULL);
  pthread_mutex_init(&active_mutex, NULL);
  pthread_mutex_lock(&active_mutex);
}

void scheduler_exit_task() {
  int tid = active_task->tid;
  active_task = NULL;
  task_descriptor_t *task = &ctx->descriptors[tid];
  task->state = STATE_ZOMBIE;
  log_debug("ST  t%d signal kernel (exit)\n\r", task->tid);
  pthread_cond_signal(&kernel_cv);
  log_debug("ST  t%d release mutex (exit)\n\r", task->tid);
  pthread_mutex_unlock(&active_mutex);
  log_debug("ST  t%d thread kill\n\r", task->tid);
  pthread_exit(NULL);
}

void scheduler_reschedule_the_world() {
  int tid = active_task->tid;
  task_descriptor_t *task = &ctx->descriptors[tid];
  log_debug("ST  t%d signal kernel\n\r", tid);
  pthread_cond_signal(&kernel_cv);
  active_task = NULL;
  if (task->state == STATE_ACTIVE) task->state = STATE_READY;
  log_debug("ST  t%d cv wait\n\r", tid);
  while (task->state != STATE_ACTIVE) pthread_cond_wait(&task_cvs[tid], &active_mutex);
  log_debug("ST  t%d cv woke\n\r", tid);
}

void *scheduler_start_task(void *td) {
  task_descriptor_t *task = (task_descriptor_t *) td;
  log_debug("ST  t%d acquire mutex\n\r", task->tid);
  pthread_mutex_lock(&active_mutex);
  task->entrypoint();
  scheduler_exit_task();
  return NULL;
}

void scheduler_activate_task(task_descriptor_t *task) {
  active_task = task;
  if (!task_started[task->tid]) {
    log_debug("SK  k create thread tid=%d\n\r", task->tid);
    pthread_create(&tasks[task->tid], NULL, &scheduler_start_task, task);
    task_started[task->tid] = true;
  } else {
    pthread_cond_signal(&task_cvs[task->tid]);
  }
  log_debug("SK  k cv wait for tid=%d\n\r", task->tid);
  task->state = STATE_ACTIVE;
  while (task->state == STATE_ACTIVE) pthread_cond_wait(&kernel_cv, &active_mutex);
  log_debug("SK  k cv awake after tid=%d\n\r", task->tid);
}
