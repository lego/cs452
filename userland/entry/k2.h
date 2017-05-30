#ifndef __K2_ENTRY_H__
#define __K2_ENTRY_H__


void k2_entry_task();

#ifndef ENTRY_FUNC
#define ENTRY_FUNC k2_entry_task
#endif

#endif
