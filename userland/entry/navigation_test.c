#include <entry/navigation_test.h>
#include <basic.h>
#include <trains/navigation.h>


void navigation_test() {
  InitNavigation();

  path_t p;
  // GetPath(&p, 3, 0);
  // PrintPath(&p);

  GetPath(&p, 3, 6);
  PrintPath(&p);

  GetPath(&p, 6, 4);
  PrintPath(&p);

  bwprintf(COM2, "== Initiating navigation ==\n\r");
  Navigate(1, 1, 3, 4);

  ExitKernel();
}

void navigation_test_task() {
  Create(10, &navigation_test);
}
