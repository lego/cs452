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
  log_scheduler_task("t%d signal kernel (exit)", task->tid);
  pthread_cond_signal(&kernel_cv);
  log_scheduler_task("t%d release mutex (exit)", task->tid);
  pthread_mutex_unlock(&active_mutex);
  log_scheduler_task("t%d thread kill", task->tid);
  pthread_exit(NULL);
}

void scheduler_reschedule_the_world() {
  int tid = active_task->tid;
  task_descriptor_t *task = &ctx->descriptors[tid];
  log_scheduler_task("t%d signal kernel", tid);
  pthread_cond_signal(&kernel_cv);
  active_task = NULL;
  if (task->state == STATE_ACTIVE) task->state = STATE_READY;
  log_scheduler_task("t%d cv wait", tid);
  while (task->state != STATE_ACTIVE) pthread_cond_wait(&task_cvs[tid], &active_mutex);
  log_scheduler_task("t%d cv woke", tid);
}

void *scheduler_start_task(void *td) {
  task_descriptor_t *task = (task_descriptor_t *) td;
  log_scheduler_task("t%d acquire mutex", task->tid);
  pthread_mutex_lock(&active_mutex);
  task->entrypoint();
  scheduler_exit_task();
  return NULL;
}

kernel_request_t *scheduler_activate_task(task_descriptor_t *task) {
  active_task = task;
  if (!task_started[task->tid]) {
    log_scheduler_kern("create thread tid=%d", task->tid);
    pthread_create(&tasks[task->tid], NULL, &scheduler_start_task, task);
    task_started[task->tid] = true;
  } else {
    pthread_cond_signal(&task_cvs[task->tid]);
  }
  log_scheduler_kern("cv wait for tid=%d", task->tid);
  task->state = STATE_ACTIVE;
  while (task->state == STATE_ACTIVE) pthread_cond_wait(&kernel_cv, &active_mutex);
  log_scheduler_kern("cv awake after tid=%d", task->tid);
  return &task->current_request;
}
