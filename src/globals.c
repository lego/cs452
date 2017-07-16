#include <kern/task_descriptor.h>
#include <kern/context.h>

volatile task_descriptor_t *active_task;
context_t *ctx;
bool should_exit;
char logs[LOG_SIZE];
volatile int log_length;
unsigned int main_fp;
int next_task_starting = -1;
