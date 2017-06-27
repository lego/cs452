#include <entry/navigation_test.h>
#include <basic.h>
#include <bwio.h>
#include <trains/navigation.h>


void navigation_test() {
  InitNavigation();

  // path_t p;
  // GetPath(&p, 3, 0);
  // PrintPath(&p);

  // GetPath(&p, 3, 6);
  // PrintPath(&p);
  //
  // GetPath(&p, 6, 4);
  // PrintPath(&p);
  //
  // bwprintf(COM2, "== Initiating navigation ==\n\r");
  // Navigate(1, 1, 3, 4);

  // Navigate(1, 1, 3, 4);

  bwprintf(COM2, "Name2Node A1=%d\n\r", Name2Node("A1"));
  bwprintf(COM2, "Name2Node A2=%d\n\r", Name2Node("A2"));
  bwprintf(COM2, "Name2Node BR1=%d\n\r", Name2Node("BR1"));

  ExitKernel();
}

void navigation_test_task() {
  Create(10, &navigation_test);
}
