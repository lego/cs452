#include <bwio.h>
#include <kernel.h>
#include <trains/navigation.h>

void navigation_test() {
  InitNavigation();

#if defined(USE_TRACKTEST)
  int EN1 = Name2Node("EN1");
  int EN2 = Name2Node("EN2");
  int EN3 = Name2Node("EN3");
  int EX1 = Name2Node("EX1");
  int EX2 = Name2Node("EX2");
  int EX3 = Name2Node("EX3");
#elif defined(USE_TRACKA) || defined(USE_TRACKB)
  path_t p;
  GetMultiDestinationPath(&p, Name2Node("E14"), Name2Node("C10"), Name2Node("E14"));
  PrintPath(&p);
#endif

  ExitKernel();
}

void navigation_test_task() { Create(10, &navigation_test); }
