#ifndef __K1_ENTRY_H__
#define __K1_ENTRY_H__

void k1_entry_task();

#ifndef ENTRY_FUNC
#define ENTRY_FUNC k1_entry_task
#endif

#endif
