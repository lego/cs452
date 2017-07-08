#pragma once

#include <kernel.h>
#include <kern/task_descriptor.h>


void interrupts_init();
void interrupts_arch_init();

void interrupts_set_waiting_task(await_event_t event_type, task_descriptor_t *task);

void interrupts_clear_waiting_task(await_event_t event_type);

task_descriptor_t *interrupts_get_waiting_task(await_event_t event_type);

void interrupts_enable_irq(await_event_t event_type);
void interrupts_disable_irq(await_event_t event_type);


void interrupts_clear_all();
