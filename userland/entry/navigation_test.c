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

  #define PATH_BUF_SIZE 40
  track_node *path[PATH_BUF_SIZE];
  int path_len;

  dijkstra(EN1, EN2);
  path_len = get_path(EN1, EN2, path, PATH_BUF_SIZE);
  print_path(EN1, EN2, path, path_len);

  dijkstra(EN1, EX3);
  path_len = get_path(EN1, EX3, path, PATH_BUF_SIZE);
  print_path(EN1, EX3, path, path_len);

  dijkstra(EN1, EX2);
  path_len = get_path(EN1, EX2, path, PATH_BUF_SIZE);
  print_path(EN1, EX2, path, path_len);

  dijkstra(EN3, EX1);
  path_len = get_path(EN3, EX1, path, PATH_BUF_SIZE);
  print_path(EN3, EX1, path, path_len);

  dijkstra(EN3, EX2);
  path_len = get_path(EN3, EX2, path, PATH_BUF_SIZE);
  print_path(EN3, EX2, path, path_len);
  #elif defined(USE_TRACKA)

  int B6 = Name2Node("B6");
  int B16 = Name2Node("B16");
  int E6 = Name2Node("E6");
  int MR14 = Name2Node("MR14");

  #define PATH_BUF_SIZE 40
  track_node *path[PATH_BUF_SIZE];
  int path_len;

  dijkstra(B6, B16);
  path_len = get_path(B6, B16, path, PATH_BUF_SIZE);
  print_path(B6, B16, path, path_len);

  dijkstra(E6, MR14);
  path_len = get_path(E6, MR14, path, PATH_BUF_SIZE);
  print_path(E6, MR14, path, path_len);

  #elif defined(USE_TRACKB)
  #error No tests defined for track B
  #endif

  ExitKernel();
}

void navigation_test_task() {
  Create(10, &navigation_test);
}
