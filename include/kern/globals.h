#pragma once

#include <kern/task_descriptor.h>
#include <kern/context.h>

extern volatile task_descriptor_t *active_task;
extern context_t *ctx;
extern bool should_exit;
extern char logs[LOG_SIZE];
extern volatile int log_length;
extern unsigned int main_fp;
extern int next_task_starting;
