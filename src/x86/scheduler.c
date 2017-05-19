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
  active_task = NULL;
  pthread_cond_signal(&kernel_cv);
  pthread_mutex_unlock(&active_mutex);
  pthread_exit(NULL);
}

void scheduler_reschedule_the_world() {
  int tid = active_task->tid;
  log_scheduler("SC  t%d signal kernel", tid);
  pthread_cond_signal(&kernel_cv);
  active_task = NULL;
  log_scheduler("SC  t%d cv wait", tid);
  while (active_task == NULL || active_task->tid != tid) pthread_cond_wait(&task_cvs[tid], &active_mutex);
  log_scheduler("SC  t%d cv woke", tid);
}

void *scheduler_start_task(void *td) {
  task_descriptor_t *task = (task_descriptor_t *) td;
  log_scheduler("SC  t%d acquire mutex", task->tid);
  pthread_mutex_lock(&active_mutex);
  task->entrypoint();
  log_scheduler("SC  t%d signal kernel", task->tid);
  pthread_cond_signal(&kernel_cv);
  active_task = NULL;
  log_scheduler("SC  t%d release mutex", task->tid);
  pthread_mutex_unlock(&active_mutex);

  return NULL;
}

void scheduler_activate_task(task_descriptor_t *task) {
  active_task = task;
  if (!task_started[task->tid]) {
    log_scheduler("SC  k create thread tid=%d", task->tid);
    pthread_create(&tasks[task->tid], NULL, &scheduler_start_task, task);
    log_scheduler("SC  k cv wait for tid=%d", task->tid);
    while (active_task != NULL) pthread_cond_wait(&kernel_cv, &active_mutex);
    log_scheduler("SC  k cv awake after tid=%d", task->tid);
    task_started[task->tid] = true;
  } else {
    log_scheduler("SC  k cv wait for tid=%d", task->tid);
    pthread_cond_signal(&task_cvs[task->tid]);
    while (active_task != NULL) pthread_cond_wait(&kernel_cv, &active_mutex);
    log_scheduler("SC  k cv awake after tid=%d", task->tid);
  }
}
