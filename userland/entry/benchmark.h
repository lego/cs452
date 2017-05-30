#ifndef __BENCHMARK_H__
#define __BENCHMARK_H__


void benchmark_entry_task();

#ifndef ENTRY_FUNC
#define ENTRY_FUNC benchmark_entry_task
#endif

#endif
