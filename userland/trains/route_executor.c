#include <kernel.h>
#include <detective/sensor_detector.h>
#include <servers/clock_server.h>
#include <servers/uart_tx_server.h>
#include <track/pathing.h>
#include <trains/navigation.h>
#include <trains/reservoir.h>
#include <trains/route_executor.h>
#include <trains/train_controller.h>
#include <jstring.h>
#include <packet.h>
#include <train_command_server.h>

typedef struct {
  int train;
} route_executor_init_t;

int get_next_sensor(path_t *path, int last_sensor_no) {
  for (int i = last_sensor_no + 1; i < path->len; i++) {
    if (path->nodes[i]->type == NODE_SENSOR) {
      return i;
    }
  }
  return -1;
}

void set_up_next_detector(path_t *path, route_executor_init_t *init, int last_sensor_no, int next_sensor_no) {
  track_node *next_sensor = NULL;
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

void reserve_track_from(int train, path_t *path, reservoir_segments_t *releasing_segments, int from_node, int to_node) {

  // Create the list of segments were going to own
  // FIXME: come up with with 1) a better segment struct 2) an easier way to get
  // data out of paths
  reservoir_segments_t data;
  data.owner = train;
  data.len = to_node - from_node;
  int segments_idx = 0;
  for (int i = from_node; i < to_node; i++) {
    track_node *src = path->nodes[i];
    data.segments[segments_idx].track_node = path->nodes[i]->id;
    switch (src->type) {
    case NODE_BRANCH:
      if (src->edge[DIR_CURVED].dest == path->nodes[i + 1]) {
        data.segments[segments_idx].dir = DIR_CURVED;
      } else {
        data.segments[segments_idx].dir = DIR_STRAIGHT;
      }
      break;
    default:
      data.segments[segments_idx].dir = DIR_AHEAD;
      break;
    }

    // keep track of owned segments, storing them in a persistent segment struct
    releasing_segments->segments[segments_idx + releasing_segments->len].track_node = data.segments[segments_idx].track_node;
    releasing_segments->segments[segments_idx + releasing_segments->len].dir = data.segments[segments_idx].dir;
    segments_idx++;
  }

  Logf(EXECUTOR_LOGGING, "%d: reserving track from %s to %s", train, path->nodes[from_node]->name, path->nodes[to_node]->name);

  int result = RequestSegment(&data);
  if (result == 1) {
    Logf(EXECUTOR_LOGGING, "%d: Route executor has ended due to segment collision", train);
    Logf(EXECUTOR_LOGGING, "%d: releasing %d segments", train, releasing_segments->len);
    ReleaseSegment(releasing_segments);

    // TODO: propogate message upwards about route failure
    // in additional signal a stop to occur on the end of the reserved segment
    DestroySelf();
  }
  // Only after now do we "own" the segments, so reflect that in releasing_segments
  releasing_segments->len += data.len;
}

void route_executor_task() {
  int tid = MyTid();
  int sender;
  int status;

  route_executor_init_t init;
  path_t path;
  reservoir_segments_t releasing_segments;
  ReceiveS(&sender, init);
  ReplyN(sender);
  ReceiveS(&sender, path);
  ReplyN(sender);
  releasing_segments.owner = init.train;
  releasing_segments.len = 0;

  Logf(EXECUTOR_LOGGING, "%d: Route executor has begun. train=%d route is %s ~> %s", init.train, init.train, path.src->name, path.dest->name);
  int dist_sum = 0;
  for (int i = 0; i < path.len; i++) {
    dist_sum += path.nodes[i]->dist;
    Logf(EXECUTOR_LOGGING, "%d:   node %4s dist %5dmm", init.train, path.nodes[i]->name, dist_sum);
  }

  char request_buffer[256] __attribute__((aligned(4)));
  packet_t *packet = (packet_t *)request_buffer;
  detector_message_t *detector_msg = (detector_message_t *)request_buffer;

  // These are the sensor indices in the path node list
  int last_sensor_no = 0;
  int next_sensor_no = get_next_sensor(&path, 0);

  set_up_next_detector(&path, &init, last_sensor_no, next_sensor_no);
  reserve_track_from(init.train, &path, &releasing_segments, last_sensor_no, next_sensor_no);

  while (true) {
    status = ReceiveS(&sender, request_buffer);
    if (status < 0)
      continue;
    status = ReplyN(sender);
    switch (packet->type) {
    case SENSOR_DETECT:
      Logf(EXECUTOR_LOGGING, "%d: Detector sensor %s hit at time=%d", init.train, track[detector_msg->details].name, Time());
      last_sensor_no = next_sensor_no;
      next_sensor_no = get_next_sensor(&path, last_sensor_no);
      if (next_sensor_no == -1) {
        Logf(EXECUTOR_LOGGING, "%d: Route Executor has completed all sensors on route. Bye \\o", init.train);
        // FIXME: do proper cleanup line alerting Executor of location, stopping, etc.
        Destroy(tid);
      }
      set_up_next_detector(&path, &init, last_sensor_no, next_sensor_no);
      reserve_track_from(init.train, &path, &releasing_segments, last_sensor_no, next_sensor_no);
      break;
    default:
      KASSERT(false, "Unexpected packet type=%d", packet->type);
      break;
    }
  }
}

int CreateRouteExecutor(int priority, int train, path_t *path) {
  char task_name[40];
  jformatf(task_name, sizeof(task_name), "route executor - train %d, %s ~> %s", train, path->src->name, path->dest->name);
  int route_executor_tid = CreateWithName(priority, route_executor_task, task_name);
  route_executor_init_t init_data;
  init_data.train = train;
  SendSN(route_executor_tid, init_data);
  Send(route_executor_tid, path, sizeof(path_t), NULL, 0);
  return route_executor_tid;
}
