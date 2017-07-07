#include <stdbool.h>
#include <kern/globals.h>
#include <kern/context.h>
#include <kern/task_descriptor.h>

volatile task_descriptor_t *active_task;
context_t *ctx;
unsigned int main_fp;
bool should_exit;
char logs[LOG_SIZE];
volatile int log_length;
