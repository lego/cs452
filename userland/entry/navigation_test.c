#include <entry/navigation_test.h>
#include <basic.h>
#include <bwio.h>
#include <trains/navigation.h>


void print_path(int src, int dest, track_node **path, int path_len) {
  int i;
  if (path_len == -1) {
    bwprintf(COM2, "Path %s ~> %s does not exist.\n\r", track[src].name, track[dest].name);
    return;
  }

  bwprintf(COM2, "Path %s ~> %s dist=%d len=%d\n\r", track[src].name, track[dest].name, track[dest].dist, path_len);
  for (i = 0; i < path_len; i++) {
    bwprintf(COM2, "  node=%s\n\r", path[i]->name);
  }
}

void navigation_test() {
  InitNavigation();

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

  // bwprintf(COM2, "Name2Node A1=%d\n\r", Name2Node("A1"));
  // bwprintf(COM2, "Name2Node A2=%d\n\r", Name2Node("A2"));
  // bwprintf(COM2, "Name2Node BR1=%d\n\r", Name2Node("BR1"));

  ExitKernel();
}

void navigation_test_task() {
  Create(10, &navigation_test);
}
