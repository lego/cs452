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


int reserve_track_from_sensor(int train, int speed, path_t *path, cbuffer_t *owned_segments, int sensor_id, int next_node, int *next_unreserved_node_id) {
  if (next_node == path->dest->id) return next_node;
  int stopdist = StoppingDistance(train, speed);
  int initial_next_node = next_node;
  Logf(EXECUTOR_LOGGING, "%d: Running reservation with stopdist %3dmm for sensor %4s", train, stopdist, track[sensor_id].name);

  reservoir_segment_edges_t data;
  data.owner = train;

  *next_unreserved_node_id = get_segments_to_reserve(train, speed, &data, path, sensor_id, next_node);

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
  for (int i = 0; i < data.len; i++) {
    cbuffer_add(owned_segments, (void *) data.edges[i]);
  }
  Logf(EXECUTOR_LOGGING, "%d: Route executor has gotten %d more segments. Total now %d", train, data.len, cbuffer_size(owned_segments));

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
      buf[0] = 14;
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

int get_next_edge_dir(track_node *node) {
  switch (node->type) {
  case NODE_EXIT:
    return -1;
    break;
  case NODE_BRANCH:
    return GetSwitchState(node->num) == SWITCH_CURVED ? DIR_CURVED : DIR_STRAIGHT;
    break;
  default:
    return DIR_AHEAD;
  }
}


track_edge *get_next_edge(track_node *node) {
  switch (node->type) {
  case NODE_EXIT:
    return NULL;
    break;
  default:
    return &node->edge[get_next_edge_dir(node)];
  }
}

track_node *get_next_node(track_node *node) {
  return get_next_edge(node)->dest;
}

int reserve_next_segments(int train, int speed, cbuffer_t *owned_segments, int sensor_no) {
  int stopdist = StoppingDistance(train, speed);
  reservoir_segment_edges_t reserving;
  reserving.len = 0;
  reserving.owner = train;

  bool found_another_sensor = false;
  track_node *curr_node = get_next_node(&track[sensor_no]);
  int dist_sum = 0;
  // While all the next nodes are acquired by this sensors trigger, keep getting more
  while (dist_sum < StoppingDistance(train, speed)) {
    if (curr_node->type == NODE_EXIT) {
      break;
    } else if (!found_another_sensor && curr_node->type == NODE_SENSOR) {
      found_another_sensor = true;
      dist_sum += get_next_edge(curr_node)->dist;
    } else if (!found_another_sensor) {
    } else {
      dist_sum += get_next_edge(curr_node)->dist;
    }

    reserving.edges[reserving.len] = get_next_edge(curr_node);
    reserving.len++;

    curr_node = get_next_node(curr_node);
  }


  int result = RequestSegmentEdges(&reserving);
  if (result == 1) {
    Logf(EXECUTOR_LOGGING, "%d: failed to obtain segments", train);
    return -1;
  } else {
    // Keep track of all owned segments
    // NOTE: we only do this after a successful RequestSegment above
    for (int i = 0; i < reserving.len; i++) {
      cbuffer_add(owned_segments, (void *) reserving.edges[i]);
    }
    Logf(EXECUTOR_LOGGING, "%d: requesting %d segments. Total now %d", train, reserving.len, cbuffer_size(owned_segments));
    return 0;
  }
}

int release_past_segments(int train, cbuffer_t *owned_segments, int sensor_no) {
  reservoir_segment_edges_t releasing;
  releasing.len = 0;
  releasing.owner = train;

  track_node *sensor = &track[sensor_no];

  while (cbuffer_size(owned_segments) > 0) {
    track_edge *edge = (track_edge *) cbuffer_pop(owned_segments, NULL);
    if (edge->dest == sensor) {
      cbuffer_unpop(owned_segments, (void *) edge);
      break;
    }

    releasing.edges[releasing.len] = edge;
    releasing.len++;
  }

  ReleaseSegmentEdges(&releasing);
  Logf(EXECUTOR_LOGGING, "%d: released %d segments. Total now %d", train, releasing.len, cbuffer_size(owned_segments));

  return 0;
}

void set_switches(path_t *path, int start_node, int end_node) {
  int start_node_idx = path_idx(path, start_node);
  int end_node_idx = path_idx(path, end_node);
  for (int i = start_node_idx; i < end_node_idx - 1; i++) {
    // FIXME: this will break if the path->dest is a branch, as we assume at
    // least one node after a branch exists
    track_node *src = path->nodes[i];
    if (src->type == NODE_BRANCH) {
      if (src->edge[DIR_CURVED].dest == path->nodes[i+1]) {
        SetSwitch(src->num, SWITCH_CURVED);
      }
      else {
        SetSwitch(src->num, SWITCH_STRAIGHT);
      }
    }
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

void train_controller() {
  int requester;
  pathing_operation_t pathing_operation;
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

  while (true) {
    ReceiveS(&requester, request_buffer);
    ReplyN(requester);
    int time = Time();
    switch (packet->type) {
      case DELAY_DETECT:
        if (detector_msg->identifier == collision_restart_id) {
          collision_restart_id = -1;
          DoCommand(train_speed_task, train, lastNonzeroSpeed);
        } else if (detector_msg->identifier == stop_delay_detector_id) {
          pathing = false;
          lastSpeed = 0;
          DoCommand(train_speed_task, train, 0);
        } else {
          KASSERT(false, "Unexpected delay detector! identifier=%d ticks=%d", detector_msg->identifier, detector_msg->details);
        }
        break;
      case SENSOR_DATA:
        {
          // Set out location in the global space
          SetTrainLocation(train, sensor_data->sensor_no);

          /**
           * Velocity calibration
           */
          int velocity = 0;
          int dist = adjSensorDist(lastSensor, sensor_data->sensor_no);
          if (dist != -1) {
            int time_diff = sensor_data->timestamp - lastSensorTime;
            velocity = (dist * 100) / time_diff;
          }
          if (velocity > 0) {
            record_velocity_sample(train, lastSpeed, velocity);
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

            /**
             * Unreserving previous segment
             * FIXME: lastSensor is set to -1 upon stop / reverse :(
             * We should instead release owned segments expect for our current one
             */
            if (lastSensor != -1) {
              release_past_segments(train, &owned_segments, sensor_data->sensor_no);
            }

            /**
             * Reservation for current segment
             */
            int result = reserve_next_segments(train, lastSpeed, &owned_segments, sensor_data->sensor_no);
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

            // Figure out next sensor to expect
            next_expected_sensor = get_next_sensor(&path, sensor_data->sensor_no);

            // Release all of the track up until this sensor
            release_past_segments(train, &owned_segments, sensor_data->sensor_no);

            // Reserve more track based on this sensor trigger
            last_unreserved_node = next_unreserved_node;
            int status = reserve_track_from_sensor(train, lastSpeed, &path, &owned_segments, sensor_data->sensor_no, next_unreserved_node, &next_unreserved_node);

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
              set_switches(&path, last_unreserved_node, next_unreserved_node);

              // If we need to navigate and there are no more sensors within stopdist
              // then we calculate a delay based off of this sensor
              if (pathing_operation == OPERATION_NAVIGATE && next_unreserved_node == path.len && !sent_navigation_stop_delay) {
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
        switch (msg->type) {
        case TRAIN_CONTROLLER_SET_SPEED:
          Logf(EXECUTOR_LOGGING, "TC executing speed cmd");
          lastSpeed = msg->speed;
          lastSensor = -1;
          DoCommand(train_speed_task, train, msg->speed);
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

        int next_expected_sensor = get_next_sensor(&path, path.src->id);
        // FIXME: using path.src->id here assumes that the first node is always a sensor.
        // This works because we only assume our location is set to sensors
        last_unreserved_node = path.src->id;
        int result = reserve_track_from_sensor(train, lastSpeed, &path, &owned_segments, path.src->id, path.nodes[1]->id, &next_unreserved_node);

        // FIXME: deal with failure result
        if (result == -1) {
          // Try pathing from the reverse?
        } else {
          // Flip switches for any branches on the recently reserved segments
          set_switches(&path, path.src->id, next_unreserved_node);

          // If we need to navigate and there are no more sensors within stopdist
          // then we calculate a delay based off of this sensor
          if (pathing_operation == OPERATION_NAVIGATE && next_unreserved_node == path.dest->id) {
            sent_navigation_stop_delay = true;
            sent_navigation_stop_delay = do_navigation_stop(&path, path.src->id, train, lastSpeed);
          } else {
            DoCommand(train_speed_task, train, navigate_msg->speed);
          }
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
  // Logf(EXECUTOR_LOGGING, "alerting train=%d sensor %4s timestamp=%d tid=%d", train, track[sensor_no].name, timestamp, train_controllers[train]);
  sensor_data_t data;
  data.packet.type = SENSOR_DATA;
  data.sensor_no = sensor_no;
  data.timestamp = timestamp;
  SendSN(train_controllers[train], data);
}

void TellTrainController(int train, int type, int speed) {
  Logf(EXECUTOR_LOGGING, "telling train=%d", train);
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
