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

void init_tracktest_route_table() {
  // MR, BR => MR, BR
  route_table[0][0] = NULL;
  route_table[0][1] = NULL;
  route_table[1][0] = NULL;
  route_table[1][1] = NULL;

  // MR, BR => EN1, EX1
  route_table[0][2] = &track[1].edge[DIR_STRAIGHT];
  route_table[0][3] = &track[1].edge[DIR_STRAIGHT];
  route_table[1][2] = &track[1].edge[DIR_STRAIGHT];
  route_table[1][3] = &track[1].edge[DIR_STRAIGHT];

  // MR, BR => EN2, EX2
  route_table[0][4] = &track[1].edge[DIR_CURVED];
  route_table[0][5] = &track[1].edge[DIR_CURVED];
  route_table[1][4] = &track[1].edge[DIR_CURVED];
  route_table[1][5] = &track[1].edge[DIR_CURVED];

  // MR, BR => EN3, EX3
  route_table[0][6] = &track[0].edge[DIR_AHEAD];
  route_table[0][7] = &track[0].edge[DIR_AHEAD];
  route_table[1][6] = &track[0].edge[DIR_AHEAD];
  route_table[1][7] = &track[0].edge[DIR_AHEAD];



  // EN1, EX1 => EN1, EX1
  route_table[2][2] = NULL;
  route_table[2][3] = NULL;
  route_table[3][2] = NULL;
  route_table[3][3] = NULL;

  // EN1, EX1 => *
  route_table[2][0] = &track[2].edge[DIR_AHEAD];
  route_table[2][1] = &track[2].edge[DIR_AHEAD];
  route_table[3][0] = &track[2].edge[DIR_AHEAD];
  route_table[3][1] = &track[2].edge[DIR_AHEAD];
  route_table[2][4] = &track[2].edge[DIR_AHEAD];
  route_table[2][5] = &track[2].edge[DIR_AHEAD];
  route_table[3][4] = &track[2].edge[DIR_AHEAD];
  route_table[3][5] = &track[2].edge[DIR_AHEAD];
  route_table[2][6] = &track[2].edge[DIR_AHEAD];
  route_table[2][7] = &track[2].edge[DIR_AHEAD];
  route_table[3][6] = &track[2].edge[DIR_AHEAD];
  route_table[3][7] = &track[2].edge[DIR_AHEAD];



  // EN2, EX2 => EN2, EX2
  route_table[4][4] = NULL;
  route_table[4][5] = NULL;
  route_table[5][4] = NULL;
  route_table[5][5] = NULL;

  // EN2, EX2 => 4
  route_table[4][0] = &track[4].edge[DIR_AHEAD];
  route_table[4][1] = &track[4].edge[DIR_AHEAD];
  route_table[5][0] = &track[4].edge[DIR_AHEAD];
  route_table[5][1] = &track[4].edge[DIR_AHEAD];
  route_table[4][2] = &track[4].edge[DIR_AHEAD];
  route_table[4][3] = &track[4].edge[DIR_AHEAD];
  route_table[5][2] = &track[4].edge[DIR_AHEAD];
  route_table[5][3] = &track[4].edge[DIR_AHEAD];
  route_table[4][6] = &track[4].edge[DIR_AHEAD];
  route_table[4][7] = &track[4].edge[DIR_AHEAD];
  route_table[5][6] = &track[4].edge[DIR_AHEAD];
  route_table[5][7] = &track[4].edge[DIR_AHEAD];


  // EN3, EX3 => EN3, EX3
  route_table[6][6] = NULL;
  route_table[6][7] = NULL;
  route_table[7][6] = NULL;
  route_table[7][7] = NULL;

  // EN3, EX3 => *
  route_table[6][0] = &track[6].edge[DIR_AHEAD];
  route_table[6][1] = &track[6].edge[DIR_AHEAD];
  route_table[7][0] = &track[6].edge[DIR_AHEAD];
  route_table[7][1] = &track[6].edge[DIR_AHEAD];
  route_table[6][4] = &track[6].edge[DIR_AHEAD];
  route_table[6][5] = &track[6].edge[DIR_AHEAD];
  route_table[7][4] = &track[6].edge[DIR_AHEAD];
  route_table[7][5] = &track[6].edge[DIR_AHEAD];
  route_table[6][2] = &track[6].edge[DIR_AHEAD];
  route_table[6][3] = &track[6].edge[DIR_AHEAD];
  route_table[7][2] = &track[6].edge[DIR_AHEAD];
  route_table[7][3] = &track[6].edge[DIR_AHEAD];
}

// FIXME: fixtures while testing
void ReverseTrainStub(int train) {
  bwprintf(COM2, "reversing train=%d\n\r", train);
}

void SetTrainSpeedStub(int train, int speed) {
  bwprintf(COM2, "setting speed=%d train=%d\n\r", speed, train);
}

void Init() {
  int i;

  #if defined(USE_TRACKA)
  init_tracka(track);
  #elif defined(USE_TRACKB)
  init_trackb(track);
  #elif defined(USE_TRACKTEST)
  init_tracktest(track);
  #else
  #error Bad TRACK value provided to Makefile. Expected "A", "B", or "TEST"
  #endif

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

  #if defined(USE_TRACKTEST)
  init_tracktest_route_table();
  #else
  #error No route table generation implemented.
  #endif



  for (i = 0; i < TRAINS_MAX; i++) {
    state.train_locations[i] = -1;
  }
}

int WhereAmI(int train) {
  return state.train_locations[train];
}

void GetPath(path_t *p, int src, int dest) {
  track_edge *next_edge = NULL;
  track_node *src_node = &track[src];
  track_node *dest_node = &track[dest];
  track_node *curr_node = src_node;

  p->nodes[0] = src_node;
  p->dist = 0;
  p->edge_count = 0;
  while ((next_edge = route_table[src_node->id][dest_node->id]) != NULL) {
    p->dist += next_edge->dist;
    p->edges[p->edge_count++] = next_edge;
    p->nodes[p->edge_count] = next_edge->dest;
  }
}

void Navigate(int train, int dest) {
  int src = WhereAmI(train);
  track_node *src_node = &track[src];
  track_node *dest_node = &track[dest];
  path_t p;
  GetPath(&p, src, dest);

  // This piece of code gets all of the turn-out stops we need to make
  // this is whenever we need to cross a turn-out and flip a switch
  track_node *breaks[TRACK_MAX]; // FIXME: come up with a reasonable upper bound
  int break_count = 0;
  int i;
  for (i = 0; i < p.edge_count; i++) {
    if (p.nodes[i]->type == NODE_BRANCH && p.edges[i]->src == p.nodes[i]) {
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
    travel_time = Move(1, 1, curr_node->id, next_node->id);
    Delay((travel_time / 10) + 1);
    curr_node = next_node;
  }
  travel_time = Move(1, 1, curr_node->id, dest_node->id);
  Delay((travel_time / 10) + 1);
  // DONE!
}

int Move(int train, int speed, int src, int dest) {
  path_t p;
  GetPath(&p, src, dest);
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
