#include <kernel.h>
#include <jstring.h>
#include <packet.h>
#include <detective/sensor_timeout_detective.h>
#include <servers/clock_server.h>
#include <track/pathing.h>
#include <trains/route_executor.h>
#include <trains/navigation.h>
#include <train_controller.h>

typedef struct {
  int train;
  int speed;
} route_executor_init_t;

int get_next_sensor(path_t * path, int last_sensor_no) {
  for (int i = last_sensor_no + 1; i < path->len; i++) {
    if (path->nodes[i]->type == NODE_SENSOR) {
      return i;
    }
  }
  return -1;
}

int set_up_next_detective(path_t * path, route_executor_init_t * init, int last_sensor_no, int next_sensor_no, int offset) {
  track_node * next_sensor = NULL;
  if (next_sensor_no != -1) {
    next_sensor = path->nodes[next_sensor_no];
  }

  // FIXME: handle case where there are no sensor to destination
  KASSERT(next_sensor != NULL, "No next sensor on route. Can't do short stuff!");

  // Find ETA to node
  track_node * last_sensor = path->nodes[last_sensor_no];
  int dist_to_node = next_sensor->dist - last_sensor->dist;
  // FIXME: deal with hardcoded speed, this was for testing purposes
  int eta_to_node = (dist_to_node * 100) / Velocity(init->train, init->speed);
  // Additional offset, possibly for acceleration and randomness
  eta_to_node += offset;
  // Generate friendly name for detective, because why not
  char name[70];
  jformatf(name, sizeof(name), "STDtcv train=%d %s ~> %s. %s or %dms", init->train, path->src->name, path->dest->name, next_sensor->name, eta_to_node * 10);

  // Boom! Off to the races
  StartSensorTimeoutDetective(name, MyTid(), eta_to_node, next_sensor->id);

  return eta_to_node;
}

void route_executor_task() {
  int tid = MyTid();
  int sender;
  int status;

  route_executor_init_t init;
  path_t path;
  ReceiveS(&sender, init);
  ReplyN(sender);
  ReceiveS(&sender, path);
  ReplyN(sender);

  RecordLogf("Route executor has begun. train=%d route is %s ~> %s\n\r", init.train, path.src->name, path.dest->name);
  int dist_sum = 0;
  for (int i = 0; i < path.len; i++) {
    dist_sum += path.nodes[i]->dist;
    RecordLogf("  node %3s dist %4dmm\n\r", path.nodes[i]->name, dist_sum);
  }

  char request_buffer[256] __attribute__ ((aligned (4)));
  packet_t * packet = (packet_t *) request_buffer;
  sensor_timeout_message_t * sensor_timeout_msg = (sensor_timeout_message_t *) request_buffer;

  // Kick off first expected sensor detective
  // provide a 2000ms grace for acceleration
  int last_sensor_no = 0;
  int next_sensor_no = get_next_sensor(&path, 0);
  int eta_to_node = set_up_next_detective(&path, &init, last_sensor_no, next_sensor_no, 200);
  int current = Time();

  SetTrainSpeed(init.train, init.speed);

  while (true) {
    status = ReceiveS(&sender, request_buffer);
    if (status < 0) continue;
    status = ReplyN(sender);
    switch (packet->type) {
      case SENSOR_TIMEOUT_DETECTIVE:
        if (sensor_timeout_msg->action == DETECTIVE_TIMEOUT) {
          RecordLogf("Detective timed out at uptime=%d\n\r", Time());
        } else if (sensor_timeout_msg->action == DETECTIVE_SENSOR) {
          int hit_time = Time();
          RecordLogf("Detective sensor hit at time=%d expected=%d, offset=%d\n\r", hit_time, current + eta_to_node, hit_time - current - eta_to_node);
        }
        last_sensor_no = next_sensor_no;
        next_sensor_no = get_next_sensor(&path, last_sensor_no);
        if (next_sensor_no != -1) {
          set_up_next_detective(&path, &init, last_sensor_no, next_sensor_no, 0);
        } else {
          RecordLogf("Route Executor has completed all sensors on route. Bye \\o\n\r");
          // FIXME: do proper cleanup line alerting Executor of location, stopping, etc.
          Destroy(tid);
        }
        current = Time();
        break;
      default:
        KASSERT(false, "Unexpected packet type=%d", packet->type);
        break;
    }
  }
}

int CreateRouteExecutor(int priority, int train, int speed, path_t * path) {
  char task_name[40];
  jformatf(task_name, sizeof(task_name), "route executor - train %d, %s ~> %s", train, path->src->name, path->dest->name);
  int route_executor_tid = CreateWithName(priority, route_executor_task, task_name);
  route_executor_init_t init_data;
  init_data.train = train;
  init_data.speed = speed;
  SendSN(route_executor_tid, init_data);
  Send(route_executor_tid, path, sizeof(path_t), NULL, 0);
  return route_executor_tid;
}
