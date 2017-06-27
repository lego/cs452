#include <trains/navigation.h>
#include <trains/track_data.h>
#include <trains/track_node.h>
#include <clock_server.h>

#include <basic.h>

track_node track[TRACK_MAX];

typedef struct TrainState {
  int train_locations[TRAINS_MAX];
} train_state_t;

train_state_t state;

int path_buf[TRACK_MAX];

track_edge *route_table[TRACK_MAX][TRACK_MAX];
int route_table_dist[TRACK_MAX][TRACK_MAX];

// FIXME: fixtures while testing
void ReverseTrainStub(int train) {
  bwprintf(COM2, "reversing train=%d\n\r", train);
}

void SetTrainSpeedStub(int train, int speed) {
  bwprintf(COM2, "setting speed=%d train=%d\n\r", speed, train);
}

int Name2Node(char *name) {
  #if defined(USE_TRACKA)
  return init_tracka_name_to_node(name);
  #elif defined(USE_TRACKB)
  return init_trackb_name_to_node(name);
  #elif defined(USE_TRACKTEST)
  return init_tracktest_name_to_node(name);
  #endif
}

void InitNavigation() {
  int i;

  #if defined(USE_TRACKA)
  init_tracka(track);
  init_tracka_route_table(track, &route_table);
  #elif defined(USE_TRACKB)
  init_trackb(track);
  init_trackb_route_table(track, &route_table);
  #elif defined(USE_TRACKTEST)
  init_tracktest(track);
  init_tracktest_route_table(track, &route_table);
  #else
  #error Bad TRACK value provided to Makefile. Expected "A", "B", or "TEST"
  #endif

  // TODO: pre-compute routing table
  // This can be in the form of:
  // src,dest => edge with edge.dest == next_node
  // (so only ~800kb of table data)
  // NOTE: this table will have a NULL value if src,dest has
  // no edges between them (e.g. reverse)
  // NOTE: key assumptions
  //  - if a <reverse> b <e1> c, then routes[a][c] = e1
  //     i.e. we transparently compress reverse direction transitions


  for (i = 0; i < TRAINS_MAX; i++) {
    state.train_locations[i] = -1;
  }
}

int WhereAmI(int train) {
  return state.train_locations[train];
}

void reverse_array(void **arr, int start, int end) {
  void *temp;
  while (start < end) {
    temp = arr[start];
    arr[start] = arr[end];
    arr[end] = temp;
    start++;
    end--;
  }
}

void GetPath(path_t *p, int src, int dest) {
  track_edge *next_edge = NULL;
  track_node *src_node = &track[src];
  track_node *dest_node = &track[dest];
  track_node *curr_node = dest_node;

  p->nodes[0] = dest_node;
  p->dist = 0;
  p->edge_count = 0;
  p->src = src_node;
  p->dest = dest_node;

  next_edge = route_table[src_node->id][curr_node->id];
  while (src_node != curr_node) {
    if (next_edge == NULL && curr_node->reverse == src_node) {
      break;
    } else if (next_edge == NULL) {
      next_edge = route_table[src_node->id][curr_node->reverse->id];
    }
    curr_node = next_edge->src;
    p->dist += next_edge->dist;
    p->edges[p->edge_count++] = next_edge;
    p->nodes[p->edge_count] = curr_node;
    next_edge = route_table[src_node->id][curr_node->id];
  }

  // reverse path and node lists, since they were generated in reverse order
  reverse_array((void **) p->nodes, 0, p->edge_count);
  reverse_array((void **) p->edges, 0, p->edge_count - 1);
}

void PrintPath(path_t *p) {
  bwprintf(COM2, "Path:\n\r");
  bwprintf(COM2, "   dist=%d edge_count=%d\n\r", p->dist, p->edge_count);
  bwprintf(COM2, "   src=%s  dest=%s\n\r", p->src->name, p->dest->name);
  int i;
  for (i = 0; i < p->edge_count; i++) {
    bwprintf(COM2, "   Node[%d]=%s\n\r", i, p->nodes[i]->name);
    bwprintf(COM2, "   Edge[%d]=%s-%s dist=%d\n\r", i, p->edges[i]->src->name, p->edges[i]->dest->name, p->edges[i]->dist);
  }
  bwprintf(COM2, "   Node[%d]=%s\n\r", i, p->nodes[i]->name);
  bwprintf(COM2, "\n\r");
}

void Navigate(int train, int speed, int src, int dest) {
  // int src = WhereAmI(train);
  track_node *src_node = &track[src];
  track_node *dest_node = &track[dest];
  path_t p;
  GetPath(&p, src, dest);
  bwprintf(COM2, "== Navigating train=%d speed=%d ==\n\r", train, speed);
  PrintPath(&p);

  // This piece of code gets all of the turn-out stops we need to make
  // this is whenever we need to cross a turn-out and flip a switch
  track_node *breaks[TRACK_MAX]; // FIXME: come up with a reasonable upper bound
  int break_count = 0;
  int i;
  for (i = 1; i < p.edge_count + 1; i++) {
    if (p.nodes[i]->type == NODE_BRANCH && p.edges[i - 1]->dest == p.nodes[i]->reverse) {
      // We have a break in our pathing!
      // This means we need to move across a switch and reverse
      breaks[break_count++] = p.nodes[i]->reverse->edge[DIR_AHEAD].dest;
    }
  }


  // TODO: improvement, only navigate enough to clear the switch
  // then move as opposed to the next landmark
  int travel_time;
  track_node *next_node;
  track_node *curr_node = src_node;
  for (i = 0; i < break_count; i++) {
    next_node = breaks[i];
    travel_time = Move(train, speed, curr_node->id, next_node->id);
    Delay((travel_time / 10) + 1);
    curr_node = next_node;
  }
  travel_time = Move(train, speed, curr_node->id, dest_node->id);
  Delay((travel_time / 10) + 1);
  // DONE!
}

int Move(int train, int speed, int src, int dest) {
  path_t p;
  GetPath(&p, src, dest);
  bwprintf(COM2, "== Move train=%d speed=%d ==\n\r", train, speed);
  PrintPath(&p);

  int travel_time = CalcTime(train, speed, &p);
  int direction = GetDirection(train, &p);
  if (direction == REVERSE_DIR) {
    ReverseTrainStub(train);
  }
  SetTrainSpeedStub(train, speed);

  // TODO: flip turn-outs to match path
  // TODO: inject expected times for all nodes
  return travel_time;
}

int CalcTime(int train, int speed, path_t *p) {
  // FIXME: does not handle where the distance is too short to fully accelerate
  int remainingDist = p->dist - AccelDist(train, speed) - DeaccelDist(train, speed);
  int remainingTime = CalculateTime(remainingDist, Velocity(train, speed));
  int totalTime = remainingTime + AccelTime(train, speed) + DeaccelTime(train, speed);
  // TODO: also account for time to first byte for the train to start moving
  // otherwise we're off
  return totalTime;
}

int GetDirection(int train, path_t *p) {
  // FIXME: handle orientation of train
  return FORWARD_DIR;
}

int SumDist(path_t *p) {
  int i;
  int sum = 0;
  for (i = 0; i < p->edge_count - 1; i++) {
    sum += p->edges[i]->dist;
  }
  return sum;
}

int AccelDist(int train, int speed) {
  return CENTIMETRES(20);
}

int DeaccelDist(int train, int speed) {
  return CENTIMETRES(20);
}

int AccelTime(int train, int speed) {
  return SECONDS(3);
}

int DeaccelTime(int train, int speed) {
  return SECONDS(3);
}

// Calculate dist from velocity * time
// (10 * 10 * 1000) / 1000
// => 10*10
//
// Calculate time from dist / velocity
// 10 * 10 / (10 * 10)
// => 1000

int CalculateDistance(int velocity, int t) {
  // NOTE: assumes our fixed point time is 1 => 1 millisecond
  return (velocity * t) / 1000;
}

int CalculateTime(int distance, int velocity) {
  // NOTE: assumes our fixed point time is 1 => 1 millisecond
  return (distance * 1000) / velocity;
}

int Velocity(int train, int speed) {
  return CENTIMETRES(10);
}
