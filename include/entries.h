#pragma once

void benchmark_entry_task();
void clock_server_test();
void k1_entry_task();
void k2_entry_task();
void k3_entry_task();
void k4_entry_task();
void malloc_test_task();
void navigation_test_task();
void train_control_entry_task();
void backtrace_test_task();
void typed_message_test();

#if defined(USE_K1)
#define ENTRY_FUNC k1_entry_task
#elif defined(USE_K2)
#define ENTRY_FUNC k2_entry_task
#elif defined(USE_K3)
#define ENTRY_FUNC k3_entry_task
#elif defined(USE_K4)
#define ENTRY_FUNC k4_entry_task
#elif defined(USE_TC)
#define ENTRY_FUNC train_control_entry_task
#elif defined(USE_NAVIGATION_TEST)
#define ENTRY_FUNC navigation_test_task
#elif defined(USE_CLOCK_SERVER_TEST)
#define ENTRY_FUNC clock_server_test
#elif defined(USE_MALLOC_TEST)
#define ENTRY_FUNC malloc_test_task
#elif defined(USE_BENCHMARK)
#define ENTRY_FUNC benchmark_entry_task
#elif defined(USE_TYPED_MESSAGE_TEST)
#define ENTRY_FUNC typed_message_test_task
#elif defined(USE_BACKTRACE_TEST)
#define ENTRY_FUNC backtrace_test_task
#else
#error Bad PROJECT value provided to Makefile. Expected "K1-4", "TC1", "BENCHMARK", "CLOCK_SERVER_TEST", "NAVIGATION_TEST"
#endif
