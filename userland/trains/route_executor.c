#include <kernel.h>
#include <jstring.h>
#include <packet.h>
#include <detective/sensor_detector.h>
#include <servers/clock_server.h>
#include <servers/uart_tx_server.h>
#include <track/pathing.h>
#include <trains/route_executor.h>
#include <trains/navigation.h>
#include <train_command_server.h>
#include <trains/train_controller.h>

typedef struct {
  int train;
} route_executor_init_t;

int get_next_sensor(path_t * path, int last_sensor_no) {
  for (int i = last_sensor_no + 1; i < path->len; i++) {
    if (path->nodes[i]->type == NODE_SENSOR) {
      return i;
    }
  }
  return -1;
}

void set_up_next_detector(path_t * path, route_executor_init_t * init, int last_sensor_no, int next_sensor_no) {
  track_node * next_sensor = NULL;
  if (next_sensor_no != -1) {
    next_sensor = path->nodes[next_sensor_no];
  }

  // FIXME: handle case where there are no sensor to destination
  KASSERT(next_sensor != NULL, "No next sensor on route. Can't do short stuff!");

  // Generate friendly name for detective, because why not
  char name[70];
  jformatf(name, sizeof(name), "STDtcv train=%d %s ~> %s. %s", init->train, path->src->name, path->dest->name, next_sensor->name);

  // Boom! Off to the races
  StartSensorDetector(name, MyTid(), next_sensor->id);
}

void reserve_track_from(int from_node, int to_node) {
  Logf(EXECUTOR_LOGGING, "would have reserved track from %s to %s", track[from_node].name, track[to_node].name);
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

  Logf(EXECUTOR_LOGGING, "Route executor has begun. train=%d route is %s ~> %s", init.train, path.src->name, path.dest->name);
  int dist_sum = 0;
  for (int i = 0; i < path.len; i++) {
    dist_sum += path.nodes[i]->dist;
    Logf(EXECUTOR_LOGGING, "  node %3s dist %4dmm", path.nodes[i]->name, dist_sum);
  }

  char request_buffer[256] __attribute__ ((aligned (4)));
  packet_t * packet = (packet_t *) request_buffer;
  detector_message_t * detector_msg = (detector_message_t *) request_buffer;

  int last_sensor_no = 0;
  int next_sensor_no = get_next_sensor(&path, 0);
  int current = Time();

  while (true) {
    status = ReceiveS(&sender, request_buffer);
    if (status < 0) continue;
    status = ReplyN(sender);
    switch (packet->type) {
      case SENSOR_DETECT:
        Logf(EXECUTOR_LOGGING, "Detector sensor %s hit at time=%d", track[detector_msg->details].name, Time());
        last_sensor_no = next_sensor_no;
        next_sensor_no = get_next_sensor(&path, last_sensor_no);
        if (next_sensor_no != -1) {
          set_up_next_detector(&path, &init, last_sensor_no, next_sensor_no);
        } else {
          Logf(EXECUTOR_LOGGING, "Route Executor has completed all sensors on route. Bye \\o");
          // FIXME: do proper cleanup line alerting Executor of location, stopping, etc.
          Destroy(tid);
        }
        reserve_track_from(last_sensor_no, next_sensor_no);
        current = Time();
        break;
      default:
        KASSERT(false, "Unexpected packet type=%d", packet->type);
        break;
    }
  }
}

int CreateRouteExecutor(int priority, int train, path_t * path) {
  char task_name[40];
  jformatf(task_name, sizeof(task_name), "route executor - train %d, %s ~> %s", train, path->src->name, path->dest->name);
  int route_executor_tid = CreateWithName(priority, route_executor_task, task_name);
  route_executor_init_t init_data;
  init_data.train = train;
  SendSN(route_executor_tid, init_data);
  Send(route_executor_tid, path, sizeof(path_t), NULL, 0);
  return route_executor_tid;
}
