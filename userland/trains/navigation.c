#include <basic.h>
#include <trains/navigation.h>
#include <trains/track_data.h>
#include <trains/track_node.h>
#include <clock_server.h>
#include <bwio.h>
#include <jstring.h>
#include <heap.h>

track_node track[TRACK_MAX];

typedef struct TrainState {
  int train_locations[TRAINS_MAX];
} train_state_t;

train_state_t state;

int path_buf[TRACK_MAX];

// FIXME: fixtures while testing
void ReverseTrainStub(int train) {
  bwprintf(COM2, "reversing train=%d\n\r", train);
}

void SetTrainSpeedStub(int train, int speed) {
  bwprintf(COM2, "setting train=%d speed=%d \n\r", train, speed);
}

void SetSwitchStub(int switch_no, char dir) {
  bwprintf(COM2, "setting switch=%d dir=%d\n\r", switch_no, dir);
}

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

  #if defined(USE_TRACKA)
  init_tracka(track);
  #elif defined(USE_TRACKB)
  init_trackb(track);
  #elif defined(USE_TRACKTEST)
  init_tracktest(track);
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

void GetPath(path_t *p, int src, int dest) {
  return;
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

void print_path(int src, int dest, track_node **path, int path_len) {
  int i;
  if (path_len == -1) {
    bwprintf(COM2, "Path %s ~> %s does not exist.\n\r", track[src].name, track[dest].name);
    return;
  }

  bwprintf(COM2, "Path %s ~> %s dist=%d len=%d\n\r", track[src].name, track[dest].name, track[dest].dist, path_len);
  for (i = 0; i < path_len; i++) {
    if (i > 0 && path[i-1]->type == NODE_BRANCH) {
      char dir;
      if (path[i-1]->edge[DIR_CURVED].dest == path[i]) {
        dir = 'C';
      } else {
        dir = 'S';
      }
      bwprintf(COM2, "    switch=%d set to %c\n\r", path[i-1]->num, dir);
    }
    bwprintf(COM2, "  node=%s\n\r", path[i]->name);
  }
}

void Navigate(int train, int speed, int src, int dest) {
  int i;
  // int src = WhereAmI(train);
  track_node *src_node = &track[src];
  track_node *dest_node = &track[dest];

  #define PATH_BUF_SIZE 40
  track_node *path[PATH_BUF_SIZE];
  int path_len;
  // FIXME: need to flip the trains source if it's on a dead end
  // FIXME: need to take dest_node->reverse->id if no path is found after
  //  an extension of this may be to find the shortest path out of reverse start and/or reversed end.
  dijkstra(src, dest);
  path_len = get_path(src, dest, path, PATH_BUF_SIZE);

  bwprintf(COM2, "== Navigating train=%d speed=%d ==\n\r", train, speed);

  print_path(src, dest, path, path_len);

  for (i = 0; i < path_len; i++) {
    if (i > 0 && path[i-1]->type == NODE_BRANCH) {
      if (path[i-1]->edge[DIR_CURVED].dest == path[i]) {
        SetSwitchStub(path[i-1]->num, 'C');
      } else {
        SetSwitchStub(path[i-1]->num, 'S');
      }
    }
  }

  SetTrainSpeedStub(train, speed);

  int travel_time = CalcTime(train, speed, path, path_len);
  Delay((travel_time / 10) + 1);
  // DONE!
}

int CalcTime(int train, int speed, track_node **path, int path_len) {
  // FIXME: does not handle where the distance is too short to fully accelerate
  int remainingDist = path[path_len - 1]->dist - AccelDist(train, speed) - DeaccelDist(train, speed);
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
  bwprintf(COM2, "Path %s ~> %s dist=%d\n\r", track[src].name, track[dest].name, v->dist);
  for (i = 0; i < n; i++) {
    bwprintf(COM2, "  node=%s\n\r", path[i]->name);
  }
}
