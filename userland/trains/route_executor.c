#include <kernel.h>
#include <jstring.h>
#include <track/pathing.h>
#include <trains/route_executor.h>

typedef struct {
  int train;
} route_executor_init_t;

void route_executor_task() {
  int tid = MyTid();
  route_executor_init_t init;
  int sender;
  path_t path;
  ReceiveS(&sender, init);
  ReceiveS(&sender, path);

  // TODO: execute path
}

int CreateRouteExecutor(int priority, int train, path_t * path) {
  char task_name[40];
  jformatf(task_name, sizeof(task_name), "route executor - train %d, %s ~> %s", train, path->src->name, path->dest->name);
  int route_executor_tid = CreateWithName(priority, route_executor_task, task_name);
  route_executor_init_t init_data;
  SendSN(route_executor_tid, init_data);
  Send(route_executor_tid, path, sizeof(path_t), NULL, 0);
  return route_executor_tid;
}
