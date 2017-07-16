#include <kernel.h>
#include <jstring.h>
#include <packet.h>
#include <cbuffer.h>
#include <detective/sensor_detector.h>
#include <servers/clock_server.h>
#include <servers/uart_tx_server.h>
#include <track/pathing.h>
#include <trains/reservoir.h>
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

void reserve_track_from(int train, path_t * path, reservoir_segments_t * releasing_segments, int from_node, int to_node) {

  // Create the list of segments were going to own
  // FIXME: come up with with 1) a better segment struct 2) an easier way to get
  // data out of paths
  reservoir_segments_t data;
  data.owner = train;
  data.len = to_node - from_node;
  int segments_idx = 0;
  for (int i = from_node; i < to_node; i++) {
    track_node * src = path->nodes[i];
    data.segments[segments_idx].track_node = path->nodes[i]->id;
    switch (src->type) {
    case NODE_BRANCH:
      if (src->edge[DIR_CURVED].dest == path->nodes[i+1]) {
         data.segments[segments_idx].dir = DIR_CURVED;
      }
      else {
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

void reserve_initial_segments(int train, path_t *path, reservoir_segments_t *releasing_segments) {
  int stopdist = 450;
  Logf(EXECUTOR_LOGGING, "Getting starting reservation with stopdist %3dmm", stopdist);
  for (int i = 1; i < path->len; i++) {
    track_node * curr_node = path->nodes[i];
    for (int j = i - 1; j >= 0; j--) {
      if (j < 1) {
        // TODO: make reservation
        Logf(EXECUTOR_LOGGING, "Resv at beginning on %4s: %4s.", path->nodes[j]->name, curr_node->name);
        break;
      }
    }
  }
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
int get_sensor_before_node(path_t *path, int node_idx, int stopdist) {
  track_node *curr_node = path->nodes[node_idx];
  for (int j = 0; j < node_idx; j++) {
    if (curr_node->dist - path->nodes[j]->dist < stopdist) {
      // We need to find the sensor before this node for knowing when we need
      // to reserve and from what sensor
      for (int k = j - 1; k >= 0; k--) {
        if (k < 1) {
          return 0; // reserve from the beginning, start position
        }
        if (path->nodes[k]->type == NODE_SENSOR) {
          // reserve precisely node[idx] - node[k].dist - stopdist
          // away from node k
          return k;
        }
      }
    }
  }

  // If we didnt' catch anything
  // it's because there src ~> node[idx] is shorter than stopping dist
  return 0; // reserve from the beginning
}

segment_t segment_from_dest_node(path_t *path, int dest_node) {
  KASSERT(dest_node > 0, "The beginning of the path isn't a valid destination node for this. node %4s", path->nodes[dest_node]->name);

  segment_t segment;
  track_node *src = path->nodes[dest_node - 1];
  segment.track_node = src->id;
  switch (src->type) {
  case NODE_BRANCH:
    if (src->edge[DIR_CURVED].dest == path->nodes[dest_node]) {
       segment.dir = DIR_CURVED;
    }
    else {
      segment.dir = DIR_STRAIGHT;
    }
    break;
  default:
    segment.dir = DIR_AHEAD;
    break;
  }
  return segment;
}

/**
 * Unreserve all owned segments up until the sensor.
 * If sensor_no is -1, we unreserve all segments currently reserved
 */
void release_track_before_sensor(int train, path_t *path, cbuffer_t *owned_segments, int sensor_no) {
  reservoir_segments_t releasing;
  releasing.len = 0;
  while (cbuffer_size(owned_segments) != 0) {
    int dest_node = (int) cbuffer_pop(owned_segments, NULL);
    if (dest_node == sensor_no) {
      // We don't want to unreserve the current sensor yet, so put it back
      cbuffer_unpop(owned_segments, (void *) dest_node);
      break;
    }
    releasing.segments[releasing.len++] = segment_from_dest_node(path, dest_node);
  }
  releasing.owner = train;

  if (sensor_no == -1) {
    Logf(EXECUTOR_LOGGING, "%d: releasing all owned segments (%d)", train, releasing.len);
  } else {
    Logf(EXECUTOR_LOGGING, "%d: releasing %2d segments before sensor %4s", train, releasing.len, path->nodes[sensor_no]->name);
  }
  ReleaseSegment(&releasing);
}

int reserve_track_from_sensor(int train, path_t *path, cbuffer_t *owned_segments, int sensor_idx, int next_node) {
  int stopdist = 450;
  int initial_next_node = next_node;

  Logf(EXECUTOR_LOGGING, "%d: Running reservation with stopdist %3dmm for sensor %4s", train, stopdist, path->nodes[sensor_idx]->name);

  reservoir_segments_t data;
  data.owner = train;
  data.len = 0;

  int sensor;
  // While all the next nodes are acquired by this sensors trigger, keep getting more
  while ((sensor = get_sensor_before_node(path, next_node, stopdist)) == sensor_idx) {
    // If we care to do a delay'd reservation acquisition,
    // otherwise generously reserve it now
    int offset_from_sensor = path->nodes[next_node]->dist - path->nodes[sensor]->dist - stopdist;

    Logf(EXECUTOR_LOGGING, "%d: Resv from %4s: %4s. Specifically %4dmm after %s.", train, path->nodes[sensor]->name, path->nodes[next_node]->name, offset_from_sensor, path->nodes[sensor]->name);

    // Generously add to reserve list
    // TODO: set up a list of tuples (dist, segment), where dist is how long to wait before reserving
    data.segments[data.len] = segment_from_dest_node(path, next_node);
    data.len++;

    // Move to the next node
    next_node++;
  }

  int result = RequestSegment(&data);
  if (result == 1) {
    Logf(EXECUTOR_LOGGING, "%d: Route executor has ended due to segment collision", train);
    release_track_before_sensor(train, path, owned_segments, -1);

    // TODO: propogate message upwards about route failure
    // in additional signal a stop to occur on the end of the reserved segment
    DestroySelf();
  }

  // Keep track of all owned segments
  // NOTE: we only do this after a successful RequestSegment above
  for (int i = initial_next_node; i < next_node; i++) {
    cbuffer_add(owned_segments, (void *) i);
  }
  Logf(EXECUTOR_LOGGING, "%d: Route executor has gotten %d more segments. Total now %d", train, data.len, cbuffer_size(owned_segments));
  return next_node;
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

  // This is a list of segment dest_nodes (not actual segments)
  void * owned_segments_buffer[30];
  cbuffer_t owned_segments;
  cbuffer_init(&owned_segments, owned_segments_buffer, 30);

  Logf(EXECUTOR_LOGGING, "%d: Route executor has begun. train=%d route is %s ~> %s", init.train, init.train, path.src->name, path.dest->name);
  int dist_sum = 0;
  for (int i = 0; i < path.len; i++) {
    dist_sum += path.nodes[i]->dist;
    Logf(EXECUTOR_LOGGING, "%d:   node %4s dist %5dmm", init.train, path.nodes[i]->name, dist_sum);
  }

  char request_buffer[256] __attribute__ ((aligned (4)));
  packet_t * packet = (packet_t *) request_buffer;
  detector_message_t * detector_msg = (detector_message_t *) request_buffer;

  // These are the sensor indices in the path node list
  int last_sensor_no = 0;
  int next_sensor_no = get_next_sensor(&path, 0);

  set_up_next_detector(&path, &init, last_sensor_no, next_sensor_no);
  int next_unreserved_node = reserve_track_from_sensor(init.train, &path, &owned_segments, last_sensor_no, 1);

  while (true) {
    status = ReceiveS(&sender, request_buffer);
    if (status < 0) continue;
    status = ReplyN(sender);
    switch (packet->type) {
      case SENSOR_DETECT:
        Logf(EXECUTOR_LOGGING, "%d: Detector sensor %s hit at time=%d", init.train, track[detector_msg->details].name, Time());
        // FIXME: maybe use the data.sensor_no? Technically this is the same now,
        // but will differ if we have sensor timeouts (i.e. multiple simultaneous detectors)
        last_sensor_no = next_sensor_no;
        // FIgure out next sensor and set up detector for it
        next_sensor_no = get_next_sensor(&path, last_sensor_no);
        if (next_sensor_no == -1) {
          Logf(EXECUTOR_LOGGING, "%d: Route Executor has completed all sensors on route. Bye \\o", init.train);
          // FIXME: do proper cleanup line alerting Executor of location, stopping, etc.
          Destroy(tid);
        }
        set_up_next_detector(&path, &init, last_sensor_no, next_sensor_no);

        // Release all of the track up until this sensor
        release_track_before_sensor(init.train, &path, &owned_segments, last_sensor_no);

        // Reserve more track based on this sensor trigger
        next_unreserved_node = reserve_track_from_sensor(init.train, &path, &owned_segments, last_sensor_no, next_unreserved_node);
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
