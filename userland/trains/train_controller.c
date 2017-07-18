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
#include <trains/route_executor.h>
#include <trains/navigation.h>
#include <trains/reservoir.h>
#include <priorities.h>

int train_controllers[TRAINS_MAX];

void InitTrainControllers() {
  for (int i = 0; i < TRAINS_MAX; i++) {
    train_controllers[i] = -1;
  }
}

typedef struct {
  int train;
  int speed;
} train_task_t;

void train_speed_task() {
    train_task_t data;
    int receiver;
    ReceiveS(&receiver, data);
    ReplyN(receiver);
    char buf[2];
    buf[1] = data.train;
    if (data.speed > 0) {
      buf[0] = 14;
      Putcs(COM1, buf, 2);
      Delay(10);
    }
    buf[0] = data.speed;
    Putcs(COM1, buf, 2);
}

void reverse_train_task() {
    train_task_t data;
    int receiver;
    ReceiveS(&receiver, data);
    ReplyN(receiver);
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

void set_speed_with_task(int train, int speed) {
  int tid = CreateRecyclable(PRIORITY_TRAIN_COMMAND_TASK, train_speed_task);
  train_task_t sp;
  sp.train = train;
  sp.speed = speed;
  SendSN(tid, sp);
}

void train_controller() {
  int requester;
  char request_buffer[1024] __attribute__ ((aligned (4)));
  packet_t * packet = (packet_t *) request_buffer;
  train_command_msg_t * msg = (train_command_msg_t *) request_buffer;
  sensor_data_t * sensor_data = (sensor_data_t *) request_buffer;
  train_navigate_t * navigate_msg = (train_navigate_t *) request_buffer;
  detector_message_t * detector_msg = (detector_message_t *) request_buffer;

  // This is a list of segment dest_nodes (not actual segments)
  void * owned_segments_buffer[40] __attribute__((aligned(4)));
  cbuffer_t owned_segments;
  cbuffer_init(&owned_segments, owned_segments_buffer, 40);

  path_t navigation_data;
  // This is here as a signifier for not properly initializing this.
  navigation_data.len = 0xDEADBEEF;

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

  while (true) {
    ReceiveS(&requester, request_buffer);
    ReplyN(requester);
    int time = Time();
    switch (packet->type) {
      case DELAY_DETECT:
        KASSERT(detector_msg->identifier == collision_restart_id, "Unexpected delay detector");
        {
          collision_restart_id = -1;
          int tid = Create(PRIORITY_TRAIN_COMMAND_TASK, train_speed_task);
          train_task_t sp;
          sp.train = train;
          sp.speed = lastNonzeroSpeed;
          SendSN(tid, sp);
        }
        break;
      case SENSOR_DATA:
        {
          SetTrainLocation(train, sensor_data->sensor_no);

          /**
           * Unreserving previous segment
           * FIXME: lastSensor is set to -1 upon stop / reverse :(
           */
          if (lastSensor != -1) release_past_segments(train, &owned_segments, lastSensor);

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

          lastSensor = sensor_data->sensor_no;
          lastSensorTime = sensor_data->timestamp;
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

          /**
           * Reservation for current segment
           */
          int result = reserve_next_segments(train, lastSpeed, &owned_segments, lastSensor);
          if (result == -1 && collision_restart_id == -1) {
            lastNonzeroSpeed = lastSpeed;
            lastSpeed = 0;
            int tid = Create(PRIORITY_TRAIN_COMMAND_TASK, train_speed_task);
            train_task_t sp;
            sp.train = train;
            sp.speed = 0;
            SendSN(tid, sp);
            collision_restart_id = StartDelayDetector("collision restart", MyTid(), 100);
          }
        }
        break;
      case TRAIN_CONTROLLER_COMMAND:
        switch (msg->type) {
        case TRAIN_CONTROLLER_SET_SPEED:
          Logf(EXECUTOR_LOGGING, "TC executing speed cmd");
          lastSpeed = msg->speed;
          lastSensor = -1;
          int tid = Create(PRIORITY_TRAIN_COMMAND_TASK, train_speed_task);
          train_task_t sp;
          sp.train = train;
          sp.speed = msg->speed;
          SendSN(tid, sp);
          break;
        case TRAIN_CONTROLLER_REVERSE: {
            Logf(EXECUTOR_LOGGING, "TC executing reverse cmd");
            int tid = Create(PRIORITY_TRAIN_COMMAND_TASK, reverse_train_task);
            train_task_t rev;
            rev.train = train;
            rev.speed = lastSpeed;
            SendSN(tid, rev);
          }
          break;
        }
        break;
      case TRAIN_NAVIGATE_COMMAND:
        navigation_data = navigate_msg->path; // Persist the path
        SetTrainSpeed(train, navigate_msg->speed);
        // FIXME: priority
        CreateRouteExecutor(10, train, navigate_msg->speed, ROUTE_EXECUTOR_NAVIGATE, &navigation_data);
        break;
      case TRAIN_STOPFROM_COMMAND:
        navigation_data = navigate_msg->path; // Persist the path
        SetTrainSpeed(train, navigate_msg->speed);
        // FIXME: priority
        CreateRouteExecutor(10, train, navigate_msg->speed, ROUTE_EXECUTOR_STOPFROM, &navigation_data);
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
  if (train_controllers[train] == -1) {
    CreateTrainController(train);
  }
}

void AlertTrainController(int train, int sensor_no, int timestamp) {
  Logf(EXECUTOR_LOGGING, "alerting train=%d sensor %4s timestamp=%d", train, track[sensor_no].name, timestamp);
  ensure_train_controller(train);
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
