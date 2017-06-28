#include <basic.h>
#include <trains/navigation.h>
#include <trains/track_data.h>
#include <trains/track_node.h>
#include <servers/clock_server.h>
#include <bwio.h>
#include <servers/uart_tx_server.h>
#include <jstring.h>
#include <heap.h>
#include <train_controller.h>

track_node track[TRACK_MAX];

typedef struct TrainState {
  int train_locations[TRAINS_MAX];
} train_state_t;

train_state_t state;

int path_buf[TRACK_MAX];

int velocity[TRAINS_MAX][15];
int stopping_distance[TRAINS_MAX][15];

int Name2Node(char *name) {
  for (int i = 0; i < TRACK_MAX; i++) {
    if (jstrcmp(track[i].name, name)) {
      return i;
    }
  }
  return -1;
}

void InitNavigation() {
  int i;

  for (i = 0; i < TRAINS_MAX; i++) {
      int j;
      for (j = 0; j < 14; j++) {
        velocity[i][j] = -1;
        stopping_distance[i][j] = -1;
      }
  }

  velocity[58][14] = MILLIMETRES(580);
  stopping_distance[58][14] = MILLIMETRES(700);

  velocity[69][9] = MILLIMETRES(420);
  velocity[69][10] = MILLIMETRES(485); // super accurate
  velocity[69][11] = MILLIMETRES(515);
  velocity[69][12] = MILLIMETRES(530);
  velocity[69][13] = MILLIMETRES(575);
  velocity[69][14] = MILLIMETRES(605);

  stopping_distance[69][10] = MILLIMETRES(280 + 56 + 30); // reasonably accurate

  #if defined(USE_TRACKA)
  init_tracka(track);
  #elif defined(USE_TRACKB)
  init_trackb(track);
  #elif defined(USE_TRACKTEST)
  init_tracktest(track);
  #else
  #error Bad TRACK value provided to Makefile. Expected "A", "B", or "TEST"
  #endif

  for (i = 0; i < TRAINS_MAX; i++) {
    state.train_locations[i] = -1;
  }

  state.train_locations[58] = Name2Node("C12");
}

void set_location(int train, int location) {
  state.train_locations[train] = location;
}

int WhereAmI(int train) {
  return state.train_locations[train];
}

void GetPath(path_t *p, int src, int dest) {
  track_node *nodes[TRACK_MAX];

  int reverse_src = track[src].reverse->id;
  int reverse_dest = track[dest].reverse->id;

  if (src == reverse_dest) {
    p->len = 0;
    p->src = &track[src];
    p->dest = &track[dest];
    p->dist = 0;
  }

  dijkstra(src, dest);
  p->len = get_path(src, dest, nodes, TRACK_MAX);


  p->dist = nodes[p->len - 1]->dist;
  p->src = &track[src];
  p->dest = &track[dest];

  // // check for more optimal normal src, reverse dest
  // dijkstra(src, reverse_dest);
  // if (track[reverse_dest].dist < p->dist) {
  //   p->len = get_path(src, reverse_dest, nodes, TRACK_MAX);
  //   p->dist = nodes[p->len - 1]->dist;
  // }
  //
  // // check for more optimal reverse src, normal dest
  // dijkstra(reverse_src, dest);
  // if (track[dest].dist < p->dist) {
  //   p->len = get_path(reverse_src, dest, nodes, TRACK_MAX);
  //   p->dist = nodes[p->len - 1]->dist;
  // }
  //
  // // check for more optimal reverse src, reverse dest
  // dijkstra(reverse_src, reverse_dest);
  // if (track[reverse_dest].dist < p->dist) {
  //   p->len = get_path(reverse_src, reverse_dest, nodes, TRACK_MAX);
  //   p->dist = nodes[p->len - 1]->dist;
  // }

  int i;
  for (i = 0; i < p->len; i++) {
    p->nodes[i] = nodes[i];
  }
}

void PrintPath(path_t *p) {
  int i;
  if (p->len == -1) {
    bwprintf(COM2, "Path %s ~> %s does not exist.\n\r", p->src->name, p->dest->name);
    return;
  }

  bwprintf(COM2, "Path %s ~> %s dist=%d len=%d\n\r", p->src->name, p->dest->name, p->dist, p->len);
  for (i = 0; i < p->len; i++) {
    if (i > 0 && p->nodes[i-1]->type == NODE_BRANCH) {
      char dir;
      if (p->nodes[i-1]->edge[DIR_CURVED].dest == p->nodes[i]) {
        dir = 'C';
      } else {
        dir = 'S';
      }
      bwprintf(COM2, "    switch=%d set to %c\n\r", p->nodes[i-1]->num, dir);
    }
    bwprintf(COM2, "  node=%s\n\r", p->nodes[i]->name);
  }
}

void Navigate(int train, int speed, int src, int dest, bool include_stop) {
  if (velocity[train][speed] == -1 || stopping_distance[train][speed] == -1) {
    KASSERT(false, "Train speed / stopping distance not yet calibrated");
  }

  int i;
  // int src = WhereAmI(train);
  track_node *src_node = &track[src];
  track_node *dest_node = &track[dest];

  path_t path;
  path_t *p = &path;
  GetPath(p, src, dest);

  for (i = 0; i < p->len; i++) {
    if (i > 0 && p->nodes[i-1]->type == NODE_BRANCH) {
      if (p->nodes[i-1]->edge[DIR_CURVED].dest == p->nodes[i]) {
        SetSwitch(p->nodes[i-1]->num, SWITCH_CURVED);
      } else {
        SetSwitch(p->nodes[i-1]->num, SWITCH_STRAIGHT);
      }
    }
  }

  // FIXME: does not handle where the distance is too short to fully accelerate
  int remainingDist = p->dist - StoppingDistance(train, speed);
  int remainingTime = CalculateTime(remainingDist, Velocity(train, speed));

  Putstr(COM2, SAVE_CURSOR);
  MoveTerminalCursor(0, COMMAND_LOCATION + 5);
  Putstr(COM2, "Setting train=");
  Puti(COM2, train);
  Putstr(COM2, " speed=");
  Puti(COM2, speed);
  Putstr(COM2, " for remaining_time=");
  Puti(COM2, remainingTime);
  Putstr(COM2, "\n\r");

  SetTrainSpeed(train, speed);

  if (include_stop) {
    Delay((remainingTime / 10));

    MoveTerminalCursor(0, COMMAND_LOCATION + 6);
    Putstr(COM2, "Stopping train=");
    Puti(COM2, train);
    Putstr(COM2, "\n\r");
    Putstr(COM2, RECOVER_CURSOR);

    SetTrainSpeed(train, 0);
  }
  state.train_locations[train] = dest;
}

int GetDirection(int train, path_t *p) {
  // FIXME: handle orientation of train
  return FORWARD_DIR;
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

int StoppingDistance(int train, int speed) {
  return stopping_distance[train][speed];
}

int CalculateDistance(int velocity, int t) {
  // NOTE: assumes our fixed point time is 1 => 1 millisecond
  return (velocity * t) / 1000;
}

int CalculateTime(int distance, int velocity) {
  // NOTE: assumes our fixed point time is 1 => 1 millisecond
  return (distance * 1000) / velocity;
}

int Velocity(int train, int speed) {
  return velocity[train][speed];
}

#define INT_MAX 999999 // FIXME: proper INT_MAX

void dijkstra(int src, int dest) {
    #define HEAP_SIZE TRACK_MAX + 1
    heapnode_t heap_nodes[HEAP_SIZE];
    heap_t heap = heap_create(heap_nodes, HEAP_SIZE);
    heap_t *h = &heap;

    int i, j;
    for (i = 0; i < TRACK_MAX; i++) {
        track_node *v = &track[i];
        v->dist = INT_MAX;
        v->prev = 0;
        v->visited = false;
    }
    track_node *v = &track[src];
    v->dist = 0;
    heap_push(h, v->dist, (void *) src);
    while (heap_size(h)) {
        i = (int) heap_pop(h);
        if (i == dest)
            break;
        v = &track[i];
        v->visited = true;
        int edges_len = 0;
        track_edge *edges[2];
        switch (v->type) {
          case NODE_EXIT:
            break;
          case NODE_ENTER:
          case NODE_SENSOR:
          case NODE_MERGE:
            edges[edges_len++] = &v->edge[DIR_AHEAD];
            break;
          case NODE_BRANCH:
            edges[edges_len++] = &v->edge[DIR_STRAIGHT];
            edges[edges_len++] = &v->edge[DIR_CURVED];
            break;
          default:
            KASSERT(false, "Node type not handled. node_id=%d type=%d", v->id, v->type);
            break;
        }
        for (j = 0; j < edges_len; j++) {
            track_edge *e = edges[j];
            track_node *u = e->dest;
            if (!u->visited && v->dist + e->dist <= u->dist) {
                u->prev = i;
                u->dist = v->dist + e->dist;
                heap_push(h, u->dist, (void *) u->id);
            }
        }
    }
}

int get_path(int src, int dest, track_node **path, int path_buf_size) {
  int n;
  int i;
  track_node *v, *u;
  v = &track[dest];

  if (v->dist == INT_MAX) {
    return -1;
  }

  // Get n, the length of the path
  for (n = 1, u = v; u->dist; u = &track[u->prev], n++);
  KASSERT(path_buf_size >= n, "Path buffer wasn't large enough. Would overflowing. src=%s dest=%s dist=%d n=%d", track[src].name, track[dest].name, track[dest].dist, n);

  // follow the path backwards
  path[n - 1] = &track[dest];
  for (i = 0, u = v; u->dist; u = &track[u->prev], i++) {
    path[n - i - 2] = &track[u->prev];
  }
  return n;
}


void set_velocity(int train, int speed, int velo) {
  velocity[train][speed] = MILLIMETRES(velo);
}

void set_stopping_distance(int train, int speed, int distance) {
  stopping_distance[train][speed] = MILLIMETRES(distance);
}
