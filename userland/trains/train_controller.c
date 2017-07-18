#include <trains/train_controller.h>
#include <basic.h>
#include <util.h>
#include <bwio.h>
#include <cbuffer.h>
#include <kernel.h>
#include <servers/nameserver.h>
#include <detective/delay_detector.h>
#include <servers/clock_server.h>
#include <servers/uart_tx_server.h>
#include <train_command_server.h>
#include <trains/switch_controller.h>
#include <trains/sensor_collector.h>
#include <trains/navigation.h>
#include <trains/reservoir.h>
#include <priorities.h>

int train_controllers[TRAINS_MAX];

// If this is deadbeef, we haven't initialized (non-zero)
int train_controllers_initialized = 0xDEADBEEF;

void InitTrainControllers() {
  train_controllers_initialized = 1;
  for (int i = 0; i < TRAINS_MAX; i++) {
    train_controllers[i] = -1;
  }
}

typedef struct {
  int train;
  int speed;
} train_task_t;


#define DoCommand(command_task, train_val, spd_val) do { \
    int command_tid = Create(PRIORITY_TRAIN_COMMAND_TASK, command_task); \
    train_task_t command_msg; \
    command_msg.train = train_val; \
    command_msg.speed = spd_val; \
    SendSN(command_tid, command_msg); \
  } while(0)

int path_idx(path_t *path, int node_id) {
  for (int i = 0; i < path->len; i++) {
    if (path->nodes[i]->id == node_id) return i;
  }
  return -1;
}

int do_navigation_stop(path_t *path, int source_node, int train, int speed) {
  int distance_before_stopping = path->node_dist[path->len - 1] - path->node_dist[path_idx(path, source_node)] - StoppingDistance(train, speed);
  int wait_ticks = distance_before_stopping * 100 / Velocity(train, speed);
  Logf(EXECUTOR_LOGGING, "NavStop: dist from %4s to %4s is %dmm. Minus stopdist is %dmm. Velocity is %d. Wait ticks is %d", path->nodes[path_idx(path, source_node)]->name, path->nodes[path->len - 1]->name, path->node_dist[path->len - 1] - path->node_dist[path_idx(path, source_node)], distance_before_stopping, Velocity(train, speed), wait_ticks);
  return StartDelayDetector("route stopper", MyTid(), wait_ticks);
}
track_edge *get_next_edge_with_path(path_t *path, track_node *node) {
  int dir = node->edge[DIR_STRAIGHT].dest == path->nodes[path_idx(path, node->id) + 1] ? DIR_STRAIGHT : DIR_CURVED;
  switch (node->type) {
  case NODE_EXIT:
    return NULL;
    break;
  case NODE_BRANCH:
    return &node->edge[dir];
    break;
  default:
    return &node->edge[DIR_AHEAD];
  }
}


int get_next_sensor(path_t * path, int last_sensor_no) {
  for (int i = path_idx(path, last_sensor_no) + 1; i < path->len; i++) {
    if (path->nodes[i]->type == NODE_SENSOR) {
      return path->nodes[i]->id;
    }
  }
  return -1;
}


/**
 * Gets the sensor before a node (with a stopdist offset).
 *
 * What this means is that if we want to reserve the track portion and be sure
 * we can stop on it, we have to reserve it when we see a particular sensor trigger
 *
 * @param path     to check with
 * @param node_idx to desire the segment reservation
 * @param stopdist offset for reserving
 */
// int get_sensor_before_node(path_t *path, int node_id, int stopdist) {
//   int node_idx = path_idx(path, node_id);
//   for (int j = 0; j < node_idx; j++) {
//     if (path->node_dist[node_idx] - path->node_dist[j] < stopdist) {
//       // We need to find the sensor before this node for knowing when we need
//       // to reserve and from what sensor
//       for (int k = j - 1; k >= 0; k--) {
//         if (k < 1) {
//           return path->src->id; // reserve from the beginning, start position
//         }
//         if (path->nodes[k]->type == NODE_SENSOR) {
//           // reserve precisely node[idx] - node[k].dist - stopdist
//           // away from node k
//           return path->nodes[k]->id;
//         }
//       }
//     }
//   }
//
//   // If we didnt' catch anything
//   // it's because there src ~> node[idx] is shorter than stopping dist
//   return path->src->id; // reserve from the beginning
// }


int get_sensor_before_node(path_t *path, int node_id, int stopdist) {
  int node_idx = path_idx(path, node_id);
  for (int j = node_idx - 1; j >= 0; j--) {
    if (path->node_dist[node_idx] - path->node_dist[j] > stopdist && path->nodes[j]->type == NODE_SENSOR) {
      // We need to find the sensor before this node for knowing when we need
      // to reserve and from what sensor
      return path->nodes[j]->id;
    }
  }

  // If we didnt' catch anything
  // it's because there src ~> node[idx] is shorter than stopping dist
  return path->src->id; // reserve from the beginning
}

track_edge *segment_from_dest_node(path_t *path, int dest_node_idx) {
  KASSERT(dest_node_idx > 0, "The beginning of the path isn't a valid destination node for this. node %4s", path->nodes[dest_node_idx]->name);

  segment_t segment;
  track_node *src = path->nodes[dest_node_idx - 1];
  int dir;
  switch (src->type) {
  case NODE_BRANCH:
    if (src->edge[DIR_CURVED].dest == path->nodes[dest_node_idx]) {
       dir = DIR_CURVED;
    }
    else {
      dir = DIR_STRAIGHT;
    }
    break;
  default:
    dir = DIR_AHEAD;
    break;
  }
  return &src->edge[dir];
}

/**
 * Get segments to reserve for nodes from next_node for the stopdist criteria
 */
int get_segments_to_reserve(int train, int speed, reservoir_segment_edges_t *data, path_t *path, int sensor_id, int next_node) {
  int stopdist = StoppingDistance(train, speed);
  data->len = 0;
  int sensor;
  // While all the next nodes are acquired by this sensors trigger, keep getting more
  int next_node_idx = path_idx(path, next_node);
  while ((sensor = get_sensor_before_node(path, next_node, stopdist)) == sensor_id) {
    // If we care to do a delay'd reservation acquisition,
    // otherwise generously reserve it now
    int sensor_idx = path_idx(path, sensor);
    int offset_from_sensor = path->node_dist[next_node_idx] - path->node_dist[sensor_idx] - stopdist;

    Logf(EXECUTOR_LOGGING, "%d: Resv from %4s: %4s. Specifically %4dmm after %s.", train, track[sensor_id].name, track[next_node].name, offset_from_sensor, track[sensor_id].name);

    // Generously add to reserve list
    data->edges[data->len++] = segment_from_dest_node(path, next_node_idx);

    // Move to the next node
    next_node_idx++;
    if (next_node_idx >= path->len) break;
    next_node = path->nodes[next_node_idx]->id;
  }
  return next_node;
}


int reserve_track_from_sensor(int train, int speed, path_t *path, int sensor_id) {
  int stopdist = StoppingDistance(train, speed);
  Logf(EXECUTOR_LOGGING, "%d: Running reservation with stopdist %3dmm for sensor %4s", train, stopdist, track[sensor_id].name);

  reservoir_segment_edges_t data;
  data.owner = train;
  data.len = 0;

  int next_sensor = get_next_sensor(path, sensor_id);
  for (int i = path_idx(path, sensor_id); i < path->len; i++) {
    if (i > path_idx(path, next_sensor) && path->node_dist[path_idx(path, i)] - path->node_dist[path_idx(path, next_sensor)] > stopdist) break;
    data.edges[data.len++] = get_next_edge_with_path(path, path->nodes[i]);
  }

  int result = RequestSegmentEdges(&data);
  if (result == 1) {
    Logf(EXECUTOR_LOGGING, "%d: Route executor has ended due to segment collision", train);
    return -1;
    // Old end execution on collision
    // release_track_before_sensor(train, path, owned_segments, -1);
    //
    // // TODO: propogate message upwards about route failure
    // // in additional signal a stop to occur on the end of the reserved segment
    // int executor_tid = WhoIsEnsured(NS_EXECUTOR);
    // route_failure_t failure_msg;
    // failure_msg.packet.type = ROUTE_FAILURE;
    // failure_msg.train = train;
    // SendSN(executor_tid, failure_msg);
  }

  // Keep track of all owned segments, only after we get them
  Logf(EXECUTOR_LOGGING, "%d: Route executor has gotten %d more segments.", train, data.len);
  for (int i = 0; i < data.len; i++) {
    Logf(EXECUTOR_LOGGING, "%d:    acquired edge (dest=%4s)", train, data.edges[i]->dest->name);
  }
  return 0;
}

void train_speed_task() {
    train_task_t data;
    int receiver;
    ReceiveS(&receiver, data);
    ReplyN(receiver);
    #if !defined(DEBUG_MODE)
    char buf[2];
    buf[1] = data.train;
    if (data.speed > 0) {
      buf[0] = data.speed + 2;
      if (buf[0] > 14) {
        buf[0] = 14;
      }
      Putcs(COM1, buf, 2);
      Delay(10);
    }
    buf[0] = data.speed;
    Putcs(COM1, buf, 2);
    #else
    bwprintf(COM2, "train_speed_task: Would have set train=%d to speed=%d\n", data.train, data.speed);
    #endif
}

void reverse_train_task() {
    train_task_t data;
    int receiver;
    ReceiveS(&receiver, data);
    ReplyN(receiver);
    #if !defined(DEBUG_MODE)
    char buf[2];
    buf[0] = 0;
    buf[1] = data.train;
    Putcs(COM1, buf, 2);
    Delay(300);
    buf[0] = 15;
    Putcs(COM1, buf, 2);
    int lastSensor = WhereAmI(data.train);
    if (lastSensor >= 0) {
      RegisterTrainReverse(data.train, lastSensor);
    }
    Delay(10);
    buf[0] = data.speed;
    Putcs(COM1, buf, 2);

    #else
    bwprintf(COM2, "reverse_train_task: Would have reversed train=%d then set speed=%d\n", data.train, data.speed);
    #endif
}

int get_next_edge_dir(path_t *path, track_node *node) {
  switch (node->type) {
  case NODE_EXIT:
    return -1;
    break;
  case NODE_BRANCH:
    if (path == NULL) {
      return GetSwitchState(node->num) == SWITCH_STRAIGHT ? DIR_STRAIGHT : DIR_CURVED;
    } else {
      int i = 0; while (path->nodes[i] != node) i++;
      return node->edge[DIR_STRAIGHT].dest == path->nodes[i + 1] ? DIR_STRAIGHT : DIR_CURVED;
    }
    break;
  default:
    return DIR_AHEAD;
  }
}


track_edge *get_next_edge(path_t *path, track_node *node) {
  switch (node->type) {
  case NODE_EXIT:
    return NULL;
    break;
  default:
    return &node->edge[get_next_edge_dir(path, node)];
  }
}


track_node *get_next_node(path_t *path, track_node *node) {
  return get_next_edge(path, node)->dest;
}

// int reserve_next_segments(int train, int speed, cbuffer_t *owned_segments, int sensor_no, int last_unreserved_node, int *next_unreserved_node) {
//   int stopdist = StoppingDistance(train, speed);
//   reservoir_segment_edges_t reserving;
//   reserving.len = 0;
//   reserving.owner = train;
//
//   Logf(EXECUTOR_LOGGING, "%d: Running reservation with stopdist %3dmm for sensor %4s", train, stopdist, track[sensor_no].name);
//
//   bool found_another_sensor = false;
//   track_node *curr_node = &track[sensor_no];
//   int from_sensor_no = sensor_no;
//   int dist_sum = 0;
//   bool start_reserving = false;
//   // While all the next nodes are acquired by this sensors trigger, keep getting more
//   while (dist_sum - track[from_sensor_no].edge[DIR_AHEAD].dist < StoppingDistance(train, speed)) {
//     if (curr_node->type == NODE_EXIT) {
//       break;
//     } else if (!found_another_sensor && curr_node->type == NODE_SENSOR) {
//       found_another_sensor = true;
//       from_sensor_no = curr_node->id;
//       dist_sum += get_next_edge(curr_node)->dist;
//     } else if (!found_another_sensor) {
//     } else {
//       dist_sum += get_next_edge(curr_node)->dist;
//     }
//     if (last_unreserved_node == curr_node->id) start_reserving = true;
//     if (start_reserving) {
//       reserving.edges[reserving.len] = get_next_edge(curr_node);
//       reserving.len++;
//     }
//
//     curr_node = get_next_node(curr_node);
//   }
//
//   *next_unreserved_node = curr_node->id;
//
//   int result = RequestSegmentEdges(&reserving);
//   if (result == 1) {
//     Logf(EXECUTOR_LOGGING, "%d: failed to obtain segments", train);
//     return -1;
//   } else {
//     // Keep track of all owned segments
//     // NOTE: we only do this after a successful RequestSegment above
//     for (int i = 0; i < reserving.len; i++) {
//       cbuffer_add(owned_segments, (void *) reserving.edges[i]);
//     }
//     Logf(EXECUTOR_LOGGING, "%d: requesting %d segments. Total now %d", train, reserving.len, cbuffer_size(owned_segments));
//     for (int i = 0; i < reserving.len; i++) {
//       Logf(EXECUTOR_LOGGING, "%d:    acquired edge (dest=%4s)", train, reserving.edges[i]->dest->name);
//     }
//     return 0;
//   }
// }

int release_past_segments(int train, cbuffer_t *owned_segments, int sensor_no) {
  reservoir_segment_edges_t releasing;
  releasing.len = 0;
  releasing.owner = train;

  track_node *sensor = &track[sensor_no];

  while (cbuffer_size(owned_segments) > 0) {
    track_edge *edge = (track_edge *) cbuffer_pop(owned_segments, NULL);
    releasing.edges[releasing.len] = edge;
    releasing.len++;

    if (edge->dest == sensor) {
      break;
    }
  }

  Logf(EXECUTOR_LOGGING, "%d: released %d segments. Total now %d", train, releasing.len, cbuffer_size(owned_segments));
  for (int i = 0; i < releasing.len; i++) {
    Logf(EXECUTOR_LOGGING, "%d:    released edge (dest=%4s)", train, releasing.edges[i]->dest->name);
  }
  ReleaseSegmentEdges(&releasing);

  return 0;
}

void set_switches(int train, int speed, path_t *path, int start_node) {
  bool found_another_sensor = false;
  track_node *curr_node = &track[start_node];
  int from_sensor_no = start_node;
  int dist_sum = 0;
  // While all the next nodes are acquired by this sensors trigger, keep getting more
  while (curr_node != path->dest && dist_sum - get_next_edge(path, curr_node)->dist < StoppingDistance(train, speed)) {
    if (curr_node->type == NODE_EXIT) {
      break;
    } else if (!found_another_sensor && curr_node->type == NODE_SENSOR) {
      found_another_sensor = true;
      from_sensor_no = curr_node->id;
      dist_sum += get_next_edge(path, curr_node)->dist;
    } else if (!found_another_sensor) {
    } else {
      dist_sum += get_next_edge(path, curr_node)->dist;
    }
    if (curr_node->type == NODE_BRANCH) {
      int i = 0; while (path->nodes[i] != curr_node) i++;
      if (curr_node->edge[DIR_CURVED].dest == path->nodes[i+1]) {
        SetSwitch(curr_node->num, SWITCH_CURVED);
      } else {
        SetSwitch(curr_node->num, SWITCH_STRAIGHT);
      }
    }
    curr_node = get_next_node(path, curr_node);
  }
}

void calibrate_set_speed() {
  train_task_t data;
  int receiver;
  ReceiveS(&receiver, data);
  ReplyN(receiver);
  TellTrainController(data.train, TRAIN_CONTROLLER_SET_SPEED, data.speed);
}

bool gluInvertMatrix(const float m[16], float invOut[16]) {
  float inv[16], det;
  int i;

  inv[0] = m[5]  * m[10] * m[15] -
  m[5]  * m[11] * m[14] -
  m[9]  * m[6]  * m[15] +
  m[9]  * m[7]  * m[14] +
  m[13] * m[6]  * m[11] -
  m[13] * m[7]  * m[10];

  inv[4] = -m[4]  * m[10] * m[15] +
  m[4]  * m[11] * m[14] +
  m[8]  * m[6]  * m[15] -
  m[8]  * m[7]  * m[14] -
  m[12] * m[6]  * m[11] +
  m[12] * m[7]  * m[10];

  inv[8] = m[4]  * m[9] * m[15] -
  m[4]  * m[11] * m[13] -
  m[8]  * m[5] * m[15] +
  m[8]  * m[7] * m[13] +
  m[12] * m[5] * m[11] -
  m[12] * m[7] * m[9];

  inv[12] = -m[4]  * m[9] * m[14] +
  m[4]  * m[10] * m[13] +
  m[8]  * m[5] * m[14] -
  m[8]  * m[6] * m[13] -
  m[12] * m[5] * m[10] +
  m[12] * m[6] * m[9];

  inv[1] = -m[1]  * m[10] * m[15] +
  m[1]  * m[11] * m[14] +
  m[9]  * m[2] * m[15] -
  m[9]  * m[3] * m[14] -
  m[13] * m[2] * m[11] +
  m[13] * m[3] * m[10];

  inv[5] = m[0]  * m[10] * m[15] -
  m[0]  * m[11] * m[14] -
  m[8]  * m[2] * m[15] +
  m[8]  * m[3] * m[14] +
  m[12] * m[2] * m[11] -
  m[12] * m[3] * m[10];

  inv[9] = -m[0]  * m[9] * m[15] +
  m[0]  * m[11] * m[13] +
  m[8]  * m[1] * m[15] -
  m[8]  * m[3] * m[13] -
  m[12] * m[1] * m[11] +
  m[12] * m[3] * m[9];

  inv[13] = m[0]  * m[9] * m[14] -
  m[0]  * m[10] * m[13] -
  m[8]  * m[1] * m[14] +
  m[8]  * m[2] * m[13] +
  m[12] * m[1] * m[10] -
  m[12] * m[2] * m[9];

  inv[2] = m[1]  * m[6] * m[15] -
  m[1]  * m[7] * m[14] -
  m[5]  * m[2] * m[15] +
  m[5]  * m[3] * m[14] +
  m[13] * m[2] * m[7] -
  m[13] * m[3] * m[6];

  inv[6] = -m[0]  * m[6] * m[15] +
  m[0]  * m[7] * m[14] +
  m[4]  * m[2] * m[15] -
  m[4]  * m[3] * m[14] -
  m[12] * m[2] * m[7] +
  m[12] * m[3] * m[6];

  inv[10] = m[0]  * m[5] * m[15] -
  m[0]  * m[7] * m[13] -
  m[4]  * m[1] * m[15] +
  m[4]  * m[3] * m[13] +
  m[12] * m[1] * m[7] -
  m[12] * m[3] * m[5];

  inv[14] = -m[0]  * m[5] * m[14] +
  m[0]  * m[6] * m[13] +
  m[4]  * m[1] * m[14] -
  m[4]  * m[2] * m[13] -
  m[12] * m[1] * m[6] +
  m[12] * m[2] * m[5];

  inv[3] = -m[1] * m[6] * m[11] +
  m[1] * m[7] * m[10] +
  m[5] * m[2] * m[11] -
  m[5] * m[3] * m[10] -
  m[9] * m[2] * m[7] +
  m[9] * m[3] * m[6];

  inv[7] = m[0] * m[6] * m[11] -
  m[0] * m[7] * m[10] -
  m[4] * m[2] * m[11] +
  m[4] * m[3] * m[10] +
  m[8] * m[2] * m[7] -
  m[8] * m[3] * m[6];

  inv[11] = -m[0] * m[5] * m[11] +
  m[0] * m[7] * m[9] +
  m[4] * m[1] * m[11] -
  m[4] * m[3] * m[9] -
  m[8] * m[1] * m[7] +
  m[8] * m[3] * m[5];

  inv[15] = m[0] * m[5] * m[10] -
  m[0] * m[6] * m[9] -
  m[4] * m[1] * m[10] +
  m[4] * m[2] * m[9] +
  m[8] * m[1] * m[6] -
  m[8] * m[2] * m[5];

  det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

  if (det == 0)
    return false;

  det = 1.0 / det;

  for (i = 0; i < 16; i++)
    invOut[i] = inv[i] * det;

  return true;
}

int reserve_next_segments(path_t *path, int train, int speed, int sensor_no) {
  int stopdist = StoppingDistance(train, speed);
  reservoir_segment_edges_t reserving;
  reserving.len = 0;
  reserving.owner = train;
  Logf(EXECUTOR_LOGGING, "%d: Running reservation with stopdist %3dmm for sensor %4s", train, stopdist, track[sensor_no].name);
  bool found_another_sensor = false;
  track_node *curr_node = &track[sensor_no];
  int from_sensor_no = sensor_no;
  int dist_sum = 0;
  // While all the next nodes are acquired by this sensors trigger, keep getting more
  while ((path == NULL || curr_node != path->dest) && dist_sum - get_next_edge(path, curr_node)->dist < StoppingDistance(train, speed)) {
    if (curr_node->type == NODE_EXIT) {
      break;
    } else if (!found_another_sensor && curr_node->type == NODE_SENSOR) {
      found_another_sensor = true;
      from_sensor_no = curr_node->id;
      dist_sum += get_next_edge(path, curr_node)->dist;
    } else if (!found_another_sensor) {
    } else {
      dist_sum += get_next_edge(path, curr_node)->dist;
    }
    reserving.edges[reserving.len] = get_next_edge(path, curr_node);
    reserving.len++;
    curr_node = get_next_node(path, curr_node);
  }
  int result = RequestSegmentEdgesAndReleaseRest(&reserving);
  if (result == 1) {
    Logf(EXECUTOR_LOGGING, "%d: failed to obtain segments", train);
    return -1;
  } else {
    // Keep track of all owned segments
    // NOTE: we only do this after a successful RequestSegment above
    Logf(EXECUTOR_LOGGING, "%d: requesting %d segments.", train, reserving.len);
    for (int i = 0; i < reserving.len; i++) {
      Logf(EXECUTOR_LOGGING, "%d:    acquired edge (dest=%4s)", train, reserving.edges[i]->dest->name);
    }
    return 0;
  }
}

void train_controller() {
  int requester;
  pathing_operation_t pathing_operation = -1;
  char request_buffer[1024] __attribute__ ((aligned (4)));
  packet_t * packet = (packet_t *) request_buffer;
  train_command_msg_t * msg = (train_command_msg_t *) request_buffer;
  sensor_data_t * sensor_data = (sensor_data_t *) request_buffer;
  train_navigate_t * navigate_msg = (train_navigate_t *) request_buffer;
  detector_message_t * detector_msg = (detector_message_t *) request_buffer;

  #define MAX_OWNED_SEGMENTS 40
  // This is a list of segment dest_nodes (not actual segments)
  void * owned_segments_buffer[MAX_OWNED_SEGMENTS] __attribute__((aligned(4)));
  cbuffer_t owned_segments;
  cbuffer_init(&owned_segments, owned_segments_buffer, MAX_OWNED_SEGMENTS);

  int executor_tid = WhoIsEnsured(NS_EXECUTOR);

  int train;
  ReceiveS(&requester, train);
  ReplyN(requester);

  RegisterTrain(train);

  int lastNonzeroSpeed = 0;
  int lastSpeed = 0;
  int lastSensor = -1;
  int lastSensorTime = 0;
  reservoir_segments_t reserving;
  reserving.owner = train;
  int collision_restart_id = -1;

  /**
   * Reserving from regular motion
   */
   int next_unreserved_node_regular = -1;

  /**
   * Values for path navigation
   */
  path_t path;
  int next_expected_sensor = -1;
  bool sent_navigation_stop_delay = false;
  bool pathing = false;
  int last_unreserved_node = 0;
  int next_unreserved_node = 1;
  int stop_delay_detector_id = -1;


  Logf(PACKET_LOG_INFO, "Starting train controller (tid=%d)", MyTid());

  bool calibrating = false;
  const int numLastVelocities = 3;
  int lastVels[numLastVelocities];
  int lastVelsStart = 0;
  int nSamples = 0;
  int calibratedSpeeds[4] = { -1, -1, -1, -1 };
  int calibrationSpeeds[4] = { 5, 7, 11, 13 };
  int calibrationSpeedN = 0;
  int calibrationLockTime = 0;

  while (true) {
    ReceiveS(&requester, request_buffer);
    ReplyN(requester);
    int time = Time();
    switch (packet->type) {
      case DELAY_DETECT:
        if (detector_msg->identifier == collision_restart_id) {
          collision_restart_id = -1;
          lastSpeed = lastNonzeroSpeed;
          DoCommand(train_speed_task, train, lastNonzeroSpeed);
        } else if (detector_msg->identifier == stop_delay_detector_id) {
          pathing = false;
          lastSpeed = 0;
          DoCommand(train_speed_task, train, 0);
        } else {
          KASSERT(false, "Unexpected delay detector! identifier=%d ticks=%d. stop_delay_detector_id=%d collision_restart_id=%d", detector_msg->identifier, detector_msg->details, stop_delay_detector_id, collision_restart_id);
        }
        break;
      case SENSOR_DATA:
        {
          // Set out location in the global space
          SetTrainLocation(train, sensor_data->sensor_no);

          if (calibrating) {
            int velocity = 0;
            int dist = adjSensorDist(lastSensor, sensor_data->sensor_no);
            if (dist != -1) {
              int time_diff = sensor_data->timestamp - lastSensorTime;
              velocity = (dist * 100) / time_diff;
            }
            if (Time() > calibrationLockTime && velocity > 0) {
              record_velocity_sample(train, lastSpeed, velocity);
              int avgV = Velocity(train, lastSpeed);
              nSamples++;
              if (nSamples > 5) {
                bool same = true;
                for (int i = 0; i < numLastVelocities; i++) {
                  if (ABS(lastVels[i] - avgV) > calibrationSpeedN) {
                    same = false;
                    break;
                  }
                }
                lastVels[lastVelsStart] = avgV;
                lastVelsStart = (lastVelsStart + 1) % numLastVelocities;
                if (same) {
                  Logf(PACKET_LOG_INFO, "Calibrated train %d velocity for speed %d: %d",
                      train, lastSpeed, avgV);
                  {
                    calibratedSpeeds[calibrationSpeedN] = avgV;
                    int nextSpeed = 0;
                    if (calibrationSpeedN < 3) {
                      calibrationSpeedN += 1;
                      nextSpeed = calibrationSpeeds[calibrationSpeedN];
                    } else {
                      Logf(PACKET_LOG_INFO, "Calibration Done!");
                      for (int i = 0; i < 4; i++) {
                        Logf(PACKET_LOG_INFO, "Calibration[%d] = %d!", calibrationSpeeds[i], calibratedSpeeds[i]);
                      }
                      float coeffMat[16];
                      float invMat[16];
                      float coeffs[4];
                      for (int i = 0; i < 4; i++) {
                        float s = (float)calibrationSpeeds[i];
                        coeffMat[i*4+0] = s*s*s;
                        coeffMat[i*4+1] = s*s;
                        coeffMat[i*4+2] = s;
                        coeffMat[i*4+3] = 1;
                      }
                      gluInvertMatrix(coeffMat, invMat);
                      for (int i = 0; i < 4; i++) {
                        float total = 0.0f;
                        for (int j = 0; j < 4; j++) {
                          total += invMat[i*4+j] * (float)calibratedSpeeds[j];
                        }
                        coeffs[i] = total;
                      }
                      for (int i = 0; i < 10; i++) {
                        int x = i + 5;
                        int est = (int)(coeffs[0]*x*x*x + coeffs[1]*x*x + coeffs[2]*x + coeffs[3]);
                        Logf(PACKET_LOG_INFO, "Est[%d] = %d!", x, est);
                        set_velocity(train, x, est);
                      }
                      calibrating = false;
                    }
                    int tid = Create(PRIORITY_TRAIN_COMMAND_TASK, calibrate_set_speed);
                    train_task_t sp;
                    sp.train = train;
                    sp.speed = nextSpeed;
                    SendSN(tid, sp);
                    nSamples = 0;
                    calibrationLockTime = Time() + 400;
                    lastSensor = -1;
                  }
                }
              }
            }
            Logf(PACKET_LOG_INFO, "Train %d velocity sample at %s: %d, avg: %d",
                train, track[sensor_data->sensor_no].name,
                velocity, Velocity(train, lastSpeed));

          }
          //Logf(PACKET_LOG_INFO, "Train %d velocity sample at %s: %d, avg: %d",
          //    train, track[sensor_data->sensor_no].name,
          //    velocity, Velocity(train, lastSpeed));

          // TODO: add other offset and calibration functions
          // (or move them from interactive)
          {
            node_dist_t nd = nextSensor(sensor_data->sensor_no);
            uart_packet_fixed_size_t packet;
            packet.type = PACKET_TRAIN_LOCATION_DATA;
            packet.len = 11;
            jmemcpy(&packet.data[0], &time, sizeof(int));
            packet.data[4] = train;
            packet.data[5] = sensor_data->sensor_no;
            packet.data[6] = nd.node;
            int nextDist = nd.dist;
            int tmp = 0;
            if (nextDist > 0) {
              tmp = (10000*Velocity(train, lastSpeed)) / nextDist;
            }
            jmemcpy(&packet.data[7], &tmp, sizeof(int));
            PutFixedPacket(&packet);
          }

          if (!pathing) {
            /**
             * If we're not pathing, but we still are moving we need to reserve
             * This segment releases the previous chunk and requests the next chunk
             */

            /*
             * Reservation for current segment
             */
            int result = reserve_next_segments(NULL, train, lastSpeed, sensor_data->sensor_no);
            if (result == -1 && collision_restart_id == -1) {
              lastNonzeroSpeed = lastSpeed;
              lastSpeed = 0;
              DoCommand(train_speed_task, train, 0);
              Logf(EXECUTOR_LOGGING, "Starting collision procedure from regular train movement. train=%d", train);
              collision_restart_id = StartDelayDetector("collision restart", MyTid(), 100);
            }
          } else if (pathing_operation == OPERATION_STOPFROM && sensor_data->sensor_no == path.dest->id) {
            // We're doing a stopfrom, and this was the place we're stopping from
              Logf(EXECUTOR_LOGGING, "%d: Route Executor is doing stopfrom stopping for %4s. Bye \\o", train, path.dest->name);
              // Tell this train to stop immediately
              DoCommand(train_speed_task, train, 0);
              pathing = false;
          } else if (sensor_data->sensor_no == path.dest->id) {
            // We picked up the last sensor for this path! Woo
            Logf(EXECUTOR_LOGGING, "%d: Route Executor has completed all sensors on route. Bye \\o", train);
            pathing = false;
          // } else if (sensor_data->sensor_no != next_expected_sensor) {
          //   // We picked up the wrong sensor!
          //   // We will try 1 of 2 things:
          //   // 1) we missed a sensor (it was broken) and this is just the sensor after that
          //   // 2) this is an incorrect sensor (and route), due to a switch failure. we need to repath
          //   if (sensor_data->sensor_no == get_next_sensor(&path, next_expected_sensor)) {
          //     // Correct route! Adjust!
          //   } else {
          //     // Incorrect route! Re-path
          //   }
          } else {
            // We picked up the right sensor. Do reservations as expected

            // Reserve more track based on this sensor trigger
            int status = reserve_next_segments(&path, train, lastSpeed, sensor_data->sensor_no);

            if (status == -1) {
              // Alert the Executor that we failed so it can find us another path
              route_failure_t failure;
              failure.packet.type = ROUTE_FAILURE;
              failure.train = train;
              failure.dest_id = path.dest->id;
              SendSN(executor_tid, failure);
              pathing = false;
              lastSpeed = 0;
              DoCommand(train_speed_task, train, 0);

            } else {
              // Flip switches for any branches on the recently reserved segments
              set_switches(train, lastSpeed, &path, sensor_data->sensor_no);

              // If we need to navigate and there are no more sensors within stopdist
              // then we calculate a delay based off of this sensor
              if (pathing_operation == OPERATION_NAVIGATE && sensor_data->sensor_no == path.dest->id && !sent_navigation_stop_delay) {
                sent_navigation_stop_delay = true;
                stop_delay_detector_id = do_navigation_stop(&path, sensor_data->sensor_no, train, lastSpeed);
              }
            }
          }

          lastSensor = sensor_data->sensor_no;
          lastSensorTime = sensor_data->timestamp;
        }
        break;
      case TRAIN_CONTROLLER_COMMAND:
        pathing = false;
        switch (msg->type) {
        case TRAIN_CONTROLLER_SET_SPEED:
          Logf(EXECUTOR_LOGGING, "TC executing speed cmd");
          lastSpeed = msg->speed;
          lastSensor = -1;
          DoCommand(train_speed_task, train, msg->speed);
          break;
        case TRAIN_CONTROLLER_CALIBRATE: {
            Logf(EXECUTOR_LOGGING, "TC executing calibrate cmd");
            for (int i = 0; i < numLastVelocities; i++) {
              lastVels[i] = 0;
            }
            nSamples = 0;
            calibrationSpeedN = 0;
            DoCommand(train_speed_task, train, calibrationSpeeds[calibrationSpeedN]);
            calibrationLockTime = Time() + 400;
            calibrating = true;
          }
          break;
        case TRAIN_CONTROLLER_REVERSE: {
            Logf(EXECUTOR_LOGGING, "TC executing reverse cmd");
            DoCommand(reverse_train_task, train, lastSpeed);
          }
          break;
        default:
          KASSERT(false, "Train got unhandled command. Command type=%d", msg->type);
          break;
        }
        break;
      case TRAIN_NAVIGATE_COMMAND:
      case TRAIN_STOPFROM_COMMAND:
        path = navigate_msg->path; // Persist the path
        debugger();
        if (path.src->reverse->id == WhereAmI(train)) {
          SetTrainLocation(train, path.src->id);
          DoCommand(reverse_train_task, train, 0);
        }
        KASSERT(path.src->id == WhereAmI(train), "Path began from a different place then train. train=%d   WhereAmI %d  and path_src %s", train, WhereAmI(train), path.src->name);

        sent_navigation_stop_delay = false;
        pathing = true;
        stop_delay_detector_id = -1;
        lastSpeed = navigate_msg->speed;

        // This was put in here because at one point an invalid path was getting
        // passed down, so this is a safegaurd
        KASSERT(path.len <= 80, "Got a corrupted path. len=%d", path.len);

        if (packet->type == TRAIN_NAVIGATE_COMMAND) {
          Logf(EXECUTOR_LOGGING, "%d: Route executor has begun. train=%d route is %s ~> %s. Navigating.", train, train, path.src->name, path.dest->name);
          pathing_operation = OPERATION_NAVIGATE;
        } else if (packet->type == TRAIN_STOPFROM_COMMAND) {
          Logf(EXECUTOR_LOGGING, "%d: Route executor has begun. train=%d route is %s ~> %s. Stopfrom", train, train, path.src->name, path.dest->name);
          pathing_operation = OPERATION_STOPFROM;
        }

        int dist_sum = 0;
        for (int i = 0; i < path.len; i++) {
          dist_sum += path.node_dist[i];
          Logf(EXECUTOR_LOGGING, "%d:   node %4s dist %5dmm", train, path.nodes[i]->name, dist_sum);
        }

        // FIXME: using path.src->id here assumes that the first node is always a sensor.
        // This works because we only assume our location is set to sensors
        int result = reserve_track_from_sensor(train, lastSpeed, &path, path.src->id);

        // FIXME: deal with failure result
        if (result == -1) {
          // Try pathing from the reverse?
        } else {
          // Flip switches for any branches on the recently reserved segments
          set_switches(train, lastSpeed, &path, path.src->id);

          // If we need to navigate and there are no more sensors within stopdist
          // then we calculate a delay based off of this sensor
          // if (pathing_operation == OPERATION_NAVIGATE && next_unreserved_node == path.dest->id) {
          //   sent_navigation_stop_delay = true;
          //   stop_delay_detector_id = do_navigation_stop(&path, path.src->id, train, lastSpeed);
          // } else {
            DoCommand(train_speed_task, train, navigate_msg->speed);
          // }
        }
        break;
      default:
        KASSERT(false, "Train controller received unhandled packet. Got type=%d", packet->type);
        break;
    }
  }
}

int CreateTrainController(int train) {
  int tid = Create(PRIORITY_TRAIN_CONTROLLER, train_controller);
  train_controllers[train] = tid;
  SendSN(tid, train);
  return tid;
}

static void ensure_train_controller(int train) {
  KASSERT(train_controllers_initialized != 0xDEADEEF, "You haven't called InitTrainControllers");
  if (train_controllers[train] == -1) {
    CreateTrainController(train);
  }
}

void AlertTrainController(int train, int sensor_no, int timestamp) {
  ensure_train_controller(train);
  Logf(EXECUTOR_LOGGING, "alerting train=%d sensor %4s timestamp=%d tid=%d", train, track[sensor_no].name, timestamp, train_controllers[train]);
  sensor_data_t data;
  data.packet.type = SENSOR_DATA;
  data.sensor_no = sensor_no;
  data.timestamp = timestamp;
  SendSN(train_controllers[train], data);
}

void TellTrainController(int train, int type, int speed) {
  ensure_train_controller(train);
  train_command_msg_t msg;
  msg.packet.type = TRAIN_CONTROLLER_COMMAND;
  msg.type = type;
  msg.speed = speed;
  SendSN(train_controllers[train], msg);
}

void NavigateTrain(int train, int speed, path_t * path) {
  ensure_train_controller(train);
  Logf(EXECUTOR_LOGGING, "navigating train=%d (tid=%d)", train, train_controllers[train]);
  train_navigate_t msg;
  msg.packet.type = TRAIN_NAVIGATE_COMMAND;
  msg.speed = speed;
  msg.path = *path;
  SendSN(train_controllers[train], msg);
}

void StopTrainAt(int train, int speed, path_t * path) {
  ensure_train_controller(train);
  Logf(EXECUTOR_LOGGING, "navigating train=%d (tid=%d)", train, train_controllers[train]);
  train_navigate_t msg;
  msg.packet.type = TRAIN_STOPFROM_COMMAND;
  msg.speed = speed;
  msg.path = *path;
  SendSN(train_controllers[train], msg);
}
