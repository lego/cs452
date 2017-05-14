#ifndef __TASK_DESCRIPTOR_H__
#define __TASK_DESCRIPTOR_H__

typedef struct {
  int tid;
  void (*entrypoint)();
} TaskDescriptor;

#endif
