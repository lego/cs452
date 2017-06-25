#include <trains/navigation.h>
#include <trains/track_data.h>
#include <trains/track_node.h>

track_node track[TRACK_MAX];

typedef struct TrainState {
  int train_locations[TRAINS_MAX];
} train_state_t;

train_state_t state;

int path_buf[TRACK_MAX];

track_edge *route_table[TRACK_MAX][TRACK_MAX];

void Init() {
  int i;

  init_tracka(track);
  for (i = 0; i < TRACK_MAX; i++) {
    track[i].id = i;
  }

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

void WhereAmI(int train) {
  return state.train_locations[train];
}

path_t GetPath(int src, int dest) {
  path_t p;
  track_edge *next_edge = NULL;
  track_node *src_node = track[src];
  track_node *dest_node = track[dest];
  track_node *curr_node = src;

  p.nodes[0] = src_node;
  p.dist = 0;
  p.edge_count = 0;
  while (next_edge = route_table[src->id][dest->id]; next_edge != NULL) {
    p.dist += next_edge->dist;
    p.edges[p.edge_count++] = next_edge;
    p.nodes[p.edge_count] = next_edge->dest;
  }

  return path;
}

void Navigate(int train, track_node *dest) {
  track_node *src = WhereAmI(train);
  path_t path = GetPath(src, dest);

  // This piece of code gets all of the turn-out stops we need to make
  // this is whenever we need to cross a turn-out and flip a switch
  track_node *breaks[TRACK_MAX]; // FIXME: come up with a reasonable upper bound
  int break_count = 0;
  int i;
  for (i = 0; i < path.edge_count; i++) {
    if (p.nodes[i].type == NODE_BRANCH && p.edges[i].src == nodes[i]) {
      // We have a break in our pathing!
      // This means we need to move across a switch and reverse
      breaks[break_count++] = p.nodes[i].reverse->edge[DIR_AHEAD].dest;
    }
  }


  // TODO: improvement, only navigate enough to clear the switch
  // then move as opposed to the next landmark
  int travel_time;
  track_node *next_node;
  track_node *curr_node = src;
  for (i = 0; i < break_count; i++) {
    next_node = breaks[i];
    travel_time = Move(cur_node, next_node);
    Delay((travel_time / 10) + 1);
    cur_node = next_node;
  }
  travel_time = Move(cur_node, dest);
  Delay((travel_time / 10) + 1);
  // DONE!
}

void Move(track_node *src, track_node *dest) {
  path_t path = GetPath(src, dest);
  int travel_time = CalcTime(train, path, speed);
  int direction = GetDirection(train, path);
  if (direction == REVERSE) {
    ReverseTrain(train);
  }
  SetTrainSpeed(train, speed);

  // TODO: flip turn-outs to match path
  // TODO: inject expected times for all nodes
  return travel_time;
}

int CalcTime(int train, path_t *p, int speed) {
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
  int sum;
  for (i = 0; i < p->len - 1; i++) {
    sum += p->edges[i].dist;
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
  return (velocity * time) / 1000;
}

int CalculateTime(int distance, int velocity) {
  // NOTE: assumes our fixed point time is 1 => 1 millisecond
  return (distance * 1000) / velocity;
}

int Velocity(int train, int speed) {
  return CENTIMETRES(10);
}
