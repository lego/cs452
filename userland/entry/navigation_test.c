#include <entry/navigation_test.h>
#include <basic.h>
#include <bwio.h>
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

  bwprintf(COM2, "Name2Node EN1=%d\n\r", Name2Node("EN1"));
  bwprintf(COM2, "Name2Node EN2=%d\n\r", Name2Node("EN2"));
  bwprintf(COM2, "Name2Node EX2=%d\n\r", Name2Node("EX2"));
  bwprintf(COM2, "Name2Node EX3=%d\n\r", Name2Node("EX3"));
  path_t p;

  GetPath(&p, Name2Node("EN1"), Name2Node("EN2"));
  PrintPath(&p);

  GetPath(&p, Name2Node("EN1"), Name2Node("EX3"));
  PrintPath(&p);

  GetPath(&p, Name2Node("EN1"), Name2Node("EX2"));
  PrintPath(&p);

  GetPath(&p, Name2Node("EN3"), Name2Node("EX1"));
  PrintPath(&p);

  GetPath(&p, Name2Node("EN3"), Name2Node("EX2"));
  PrintPath(&p);
  #elif defined(USE_TRACKA)
  path_t p;
  GetPath(&p, Name2Node("B6"), Name2Node("B16"));
  PrintPath(&p);

  GetPath(&p, Name2Node("B5"), Name2Node("B16"));
  PrintPath(&p);

  GetPath(&p, Name2Node("E6"), Name2Node("MR14"));
  PrintPath(&p);
  #elif defined(USE_TRACKB)
  path_t p;
  GetPath(&p, Name2Node("D4"), Name2Node("B6"));
  PrintPath(&p);
  GetPath(&p, Name2Node("B6"), Name2Node("C12"));
  PrintPath(&p);
  GetPath(&p, Name2Node("C12"), Name2Node("A4"));
  PrintPath(&p);
  GetPath(&p, Name2Node("A4"), Name2Node("B16"));
  PrintPath(&p);
  GetPath(&p, Name2Node("B16"), Name2Node("C5"));
  PrintPath(&p);
  GetPath(&p, Name2Node("C5"), Name2Node("C15"));
  PrintPath(&p);
  GetPath(&p, Name2Node("C15"), Name2Node("D12"));
  PrintPath(&p);
  GetPath(&p, Name2Node("D12"), Name2Node("E11"));
  PrintPath(&p);
  GetPath(&p, Name2Node("E11"), Name2Node("D10"));
  PrintPath(&p);
  GetPath(&p, Name2Node("D10"), Name2Node("D5"));
  PrintPath(&p);
  GetPath(&p, Name2Node("D5"), Name2Node("E6"));
  PrintPath(&p);
  GetPath(&p, Name2Node("E6"), Name2Node("D4"));
  PrintPath(&p);


  GetPath(&p, Name2Node("C14"), Name2Node("E8"));
  PrintPath(&p);


  GetPath(&p, Name2Node("C12"), Name2Node("E8"));
  PrintPath(&p);

  GetPath(&p, Name2Node("E8"), Name2Node("C12"));
  PrintPath(&p);


  #endif

  ExitKernel();
}

void navigation_test_task() {
  Create(10, &navigation_test);
}
