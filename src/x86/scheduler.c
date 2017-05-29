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

volatile bool interrupt_being_handling;

void scheduler_arch_init() {
  interrupt_being_handling = false;
  int i;
  for (i = 0; i < MAX_TASKS; i++) {
    pthread_cond_init(&task_cvs[i], NULL);
    task_started[i] = false;
  }
  pthread_cond_init(&kernel_cv, NULL);
  pthread_mutex_init(&active_mutex, NULL);
  pthread_mutex_lock(&active_mutex);
}

void scheduler_exit_task(task_descriptor_t *task) {
  // cause the task thread to end
  // WARNING: this was taken out due to it causing failure when killing a thread
  // so for now the threads also stay alive
  task->state = STATE_ZOMBIE;
}

void scheduler_reschedule_the_world() {
  int tid = active_task->tid;
  task_descriptor_t *task = &ctx->descriptors[tid];
  log_scheduler_task("signal kernel", tid);
  pthread_cond_signal(&kernel_cv);
  active_task = NULL;
  if (task->state == STATE_ACTIVE) task->state = STATE_READY;
  log_scheduler_task("cv wait", tid);
  while (task->state != STATE_ACTIVE) pthread_cond_wait(&task_cvs[tid], &active_mutex);
  log_scheduler_task("cv woke", tid);
}

void *scheduler_start_task(void *td) {
  task_descriptor_t *task = (task_descriptor_t *) td;
  log_scheduler_task("acquire mutex", task->tid);
  pthread_mutex_lock(&active_mutex);
  task->entrypoint();

  active_task = NULL;
  log_scheduler_task("signal kernel (exit)", task->tid);
  pthread_cond_signal(&kernel_cv);
  log_scheduler_task("release mutex (exit)", task->tid);
  pthread_mutex_unlock(&active_mutex);
  log_scheduler_task("thread KILL", task->tid);
  task->state = STATE_ZOMBIE;
  pthread_exit(NULL);

  return NULL;
}

kernel_request_t *scheduler_activate_task(task_descriptor_t *task) {
  active_task = task;
  if (!task_started[task->tid]) {
    log_scheduler_kern("create thread tid=%d", task->tid);
    pthread_create(&tasks[task->tid], NULL, &scheduler_start_task, task);
    task_started[task->tid] = true;
  } else {
    log_scheduler_kern("cv signal tid=%d", task->tid);
    pthread_cond_signal(&task_cvs[task->tid]);
  }
  log_scheduler_kern("cv wait for tid=%d", task->tid);
  task->state = STATE_ACTIVE;
  while (task->state == STATE_ACTIVE) pthread_cond_wait(&kernel_cv, &active_mutex);
  log_scheduler_kern("cv awake after tid=%d", task->tid);
  return &task->current_request;
}

pthread_t scheduler_x86_get_thread(int tid) {
  return tasks[tid];
}

void scheduler_x86_deactivate_task_for_interrupt() {
  int tid = active_task->tid;
  task_descriptor_t *task = &ctx->descriptors[tid];
  log_scheduler_task("t%d signal kernel for interrupt", tid);
  pthread_cond_signal(&kernel_cv);
  active_task = NULL;
  if (task->state == STATE_ACTIVE) task->state = STATE_READY;
  interrupt_being_handling = true;
  log_scheduler_task("t%d cv wait", tid);
  while (task->state != STATE_ACTIVE) pthread_cond_wait(&task_cvs[tid], &active_mutex);
  log_scheduler_task("t%d cv woke", tid);
}
