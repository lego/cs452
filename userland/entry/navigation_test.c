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

  int BR17 = Name2Node("BR17");
  int MR13 = Name2Node("MR13");
  int BR13 = Name2Node("BR13");

  bwprintf(COM2, "Name2Node BR17=%d\n\r", BR17);
  bwprintf(COM2, "Name2Node MR13=%d\n\r", MR13);
  bwprintf(COM2, "Name2Node BR13=%d\n\r", BR13);

  #define PATH_BUF_SIZE 40
  track_node *path[PATH_BUF_SIZE];
  int path_len;

  dijkstra(BR17, MR13);
  path_len = get_path(BR17, MR13, path, PATH_BUF_SIZE);
  print_path(BR17, MR13, path, path_len);

  dijkstra(BR17, BR13);
  path_len = get_path(BR17, BR13, path, PATH_BUF_SIZE);
  print_path(BR17, BR13, path, path_len);

  #elif defined(USE_TRACKB)
  #error No tests defined for track B
  #endif

  ExitKernel();
}

void navigation_test_task() {
  Create(10, &navigation_test);
}
