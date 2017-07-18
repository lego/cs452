#include <basic.h>
#include <heap.h>
#include <jstring.h>
#include <track/pathing.h>
#include <trains/switch_controller.h>
#include <trains/reservoir.h>

bool pathing_initialized = false;

track_node track[TRACK_MAX];

void initSensorDistances();

void InitPathing() {
  int i;
  pathing_initialized = true;

  #if defined(USE_TRACKA)
  init_tracka(track);
  #elif defined(USE_TRACKB)
  init_trackb(track);
  #elif defined(USE_TRACKTEST)
  init_tracktest(track);
  #else
  #error Bad TRACK value provided to Makefile. Expected "A", "B", or "TEST"
  #endif

  initSensorDistances();
}

#define NUM_SENSORS 80
int prevSensor[NUM_SENSORS][2];
int sensorDistances[NUM_SENSORS][2];

int adjSensorDist(int last, int current) {
  if (last != -1 && current != -1) {
    for (int i = 0; i < 2; i++) {
      if (prevSensor[current][i] == last) {
        return sensorDistances[current][i];
      }
    }
  }
  return -1;
}

void initSensorDistances() {
  for (int i = 0; i < NUM_SENSORS; i++) {
    prevSensor[i][0] = -1;
    prevSensor[i][1] = -1;
    sensorDistances[i][0] = -1;
    sensorDistances[i][1] = -1;
  }

  for (int i = 0; i < 80; i++) {
    int next1 = -1;
    int next2 = -1;

    int next = findSensorOrBranch(i).node;
    if (track[next].type == NODE_BRANCH) {
      next1 = track[next].edge[DIR_STRAIGHT].dest->id;
      next2 = track[next].edge[DIR_CURVED].dest->id;
      if (track[next1].type != NODE_SENSOR) {
        next1 = findSensorOrBranch(next1).node;
      }
      if (track[next2].type != NODE_SENSOR) {
        next2 = findSensorOrBranch(next2).node;
      }
    } else {
      next1 = next;
    }

    if (next1 >= 0 && next1 < NUM_SENSORS) {
      for (int j = 0; j < 2; j++) {
        if (prevSensor[next1][j] == -1) {
          prevSensor[next1][j] = i;
          break;
        }
      }
    }
    if (next2 >= 0 && next2 < NUM_SENSORS) {
      for (int j = 0; j < 2; j++) {
        if (prevSensor[next2][j] == -1) {
          prevSensor[next2][j] = i;
          break;
        }
      }
    }
  }

  path_t p;
  for (int i = 0; i < 80; i++) {
    for (int j = 0; j < 2; j++) {
      if (prevSensor[i][j] != -1) {
        GetPath(&p, prevSensor[i][j], i);
        sensorDistances[i][j] = p.dist;
      }
    }
  }
}

node_dist_t findSensorOrBranch(int start) {
  node_dist_t nd;
  nd.node = start;
  nd.dist = 0;
  do {
    if (track[nd.node].edge[DIR_AHEAD].dest != 0) {
      nd.dist += track[nd.node].edge[DIR_AHEAD].dist;
      nd.node = track[nd.node].edge[DIR_AHEAD].dest->id;
    } else {
      nd.node = -1;
      nd.dist = 0;
    }
  } while(nd.node >= 0 && track[nd.node].type != NODE_SENSOR && track[nd.node].type != NODE_BRANCH);

  return nd;
}

node_dist_t nextSensor(int node) {
  node_dist_t nd;
  nd = findSensorOrBranch(node);
  while (nd.node >= 0 && track[nd.node].type == NODE_BRANCH) {
    int state = GetSwitchState(track[nd.node].num);
    nd.dist += track[nd.node].edge[state].dist;
    nd.node = track[nd.node].edge[state].dest->id;
    if (track[nd.node].type != NODE_SENSOR) {
      node_dist_t other_nd = findSensorOrBranch(nd.node);
      nd.dist += other_nd.dist;
      nd.node = other_nd.node;
    }
  }
  if (!(nd.node >= 0 && track[nd.node].type == NODE_SENSOR)) {
    nd.node = -1;
    nd.dist = 0;
  }
  return nd;
}

void GetMultiDestinationPath(path_t *p, int src, int dest1, int dest2) {
  path_t p1;
  path_t p2;
  GetPath(&p1, src, dest1);
  GetPath(&p2, dest1, dest2);

  p->dist = p1.dist + p2.dist;
  p->len = p1.len + p2.len - 1;
  int i;
  for (i = 0; i < p1.len; i++) {
    p->nodes[i] = p1.nodes[i];
  }
  for (i = 1; i < p2.len; i++) {
    p->nodes[i + p1.len - 1] = p2.nodes[i];
  }
  p->src = p1.src;
  p->dest = p2.dest;
}

int Name2Node(char *name) {
  KASSERT(pathing_initialized, "pathing not initialized");
  for (int i = 0; i < TRACK_MAX; i++) {
    if (jstrcmp(track[i].name, name)) {
      return i;
    }
  }
  return -1;
}

int GetReverseNode(int node) {
  KASSERT(pathing_initialized, "pathing not initialized");
  return track[node].reverse->id;
}

/**
 * This is a black magic variable we use to get "mutual exlusivity" over the
 * track graph for doing routing on. In particular, we have 5 possibly indices
 * to use for dijkstra, and so we ASSUME (!!!) we will be able to exlusively
 * increment it and then use that index.
 */
volatile int global_pathing_idx = 1;

void GetPathWithResv(path_t *p, int src, int dest, int resv_owner) {
  KASSERT(src >= 0 && dest >= 0, "Bad src or dest: got src=%d dest=%d", src, dest);
  track_node *nodes[TRACK_MAX];

  int reverse_src = track[src].reverse->id;
  int reverse_dest = track[dest].reverse->id;

  if (src == reverse_dest) {
    p->len = 0;
    p->src = &track[src];
    p->dest = &track[dest];
    p->dist = 0;
  }

  int pathing_idx = 0;

  // volatile int pathing_idx = global_pathing_idx++;
  // if (global_pathing_idx >= 5) global_pathing_idx = 0;
  // if (pathing_idx >= 5) pathing_idx = pathing_idx % 5;

  dijkstra(src, dest, pathing_idx, resv_owner);
  p->len = get_path(src, dest, nodes, TRACK_MAX, pathing_idx);
  KASSERT(p->len <= PATH_MAX, "Generated path was too large and overflowed. len=%d for %4s ~> %4s", p->len, track[src].name, track[dest].name);
  p->dist = 0;

  if (p->len > 0) {
    p->dist = nodes[p->len - 1]->p_dist[pathing_idx];
    p->src = &track[src];
    p->dest = &track[dest];
    for (int i = 0; i < p->len; i++) {
      p->nodes[i] = nodes[i];
      p->node_dist[i] = nodes[i]->p_dist[pathing_idx];
    }
  }

  // // check for more optimal normal src, reverse dest
  // dijkstra(src, reverse_dest, pathing_idx, resv_owner);
  // if (track[reverse_dest].p_dist[pathing_idx] < p->dist) {
  //   p->len = get_path(src, reverse_dest, nodes, TRACK_MAX, pathing_idx);
  //   p->dist = nodes[p->len - 1]->p_dist[pathing_idx];
  //   p->src = &track[src];
  //   p->dest = &track[reverse_dest];
  //   for (int i = 0; i < p->len; i++) {
  //     p->nodes[i] = nodes[i];
  //     p->node_dist[i] = nodes[i]->p_dist[pathing_idx];
  //   }
  // }
  // KASSERT(p->len <= PATH_MAX, "Generated path was too large and overflowed. len=%d for %4s ~> %4s", p->len, track[src].name, track[dest].name);
  //
  // // check for more optimal reverse src, normal dest
  // dijkstra(reverse_src, dest, pathing_idx, resv_owner);
  // if (track[dest].p_dist[pathing_idx] < p->dist) {
  //   p->len = get_path(reverse_src, dest, nodes, TRACK_MAX, pathing_idx);
  //   p->dist = nodes[p->len - 1]->p_dist[pathing_idx];
  //   p->src = &track[reverse_src];
  //   p->dest = &track[dest];
  //   for (int i = 0; i < p->len; i++) {
  //     p->nodes[i] = nodes[i];
  //     p->node_dist[i] = nodes[i]->p_dist[pathing_idx];
  //   }
  // }
  // KASSERT(p->len <= PATH_MAX, "Generated path was too large and overflowed. len=%d for %4s ~> %4s", p->len, track[src].name, track[dest].name);
  //
  // // check for more optimal reverse src, reverse dest
  // dijkstra(reverse_src, reverse_dest, pathing_idx, resv_owner);
  // if (track[reverse_dest].p_dist[pathing_idx] < p->dist) {
  //   p->len = get_path(reverse_src, reverse_dest, nodes, TRACK_MAX, pathing_idx);
  //   p->dist = nodes[p->len - 1]->p_dist[pathing_idx];
  //   p->src = &track[reverse_src];
  //   p->dest = &track[reverse_dest];
  //   for (int i = 0; i < p->len; i++) {
  //     p->nodes[i] = nodes[i];
  //     p->node_dist[i] = nodes[i]->p_dist[pathing_idx];
  //   }
  // }
  // KASSERT(p->len <= PATH_MAX, "Generated path was too large and overflowed. len=%d for %4s ~> %4s", p->len, track[src].name, track[dest].name);
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
    bwprintf(COM2, "  node %4s  dist=%5dmm\n\r", p->nodes[i]->name, p->node_dist[i]);
  }
}


void dijkstra(int src, int dest, int p_idx, int owner) {
    KASSERT(src >= 0 && dest >= 0, "Bad src or dest: got src=%d dest=%d", src, dest);
    #define HEAP_SIZE TRACK_MAX + 1
    heapnode_t heap_nodes[HEAP_SIZE];
    heap_t heap = heap_create(heap_nodes, HEAP_SIZE);
    heap_t *h = &heap;

    int i, j;
    for (i = 0; i < TRACK_MAX; i++) {
        track_node *v = &track[i];
        v->p_dist[p_idx] = INT_MAX;
        v->prev[p_idx] = 0;
        v->visited[p_idx] = false;
    }
    track_node *v = &track[src];
    v->p_dist[p_idx] = 0;
    heap_push(h, v->p_dist[p_idx], (void *) src);
    while (heap_size(h)) {
        i = (int) heap_pop(h);
        if (i == dest)
            break;
        v = &track[i];
        v->visited[p_idx] = true;
        int edges_len = 0;
        track_edge *edges[2];
        switch (v->type) {
          case NODE_EXIT:
            break;
          case NODE_ENTER:
          case NODE_SENSOR:
          case NODE_MERGE:
            // if (v->edge[DIR_AHEAD].owner == -1 || v->edge[DIR_AHEAD].owner == owner) {
            //   edges[edges_len++] = &v->edge[DIR_AHEAD];
            // }
            edges[edges_len++] = &v->edge[DIR_AHEAD];
            break;
          case NODE_BRANCH:
            // if (v->edge[DIR_STRAIGHT].owner == -1 || v->edge[DIR_STRAIGHT].owner == owner) {
            //   edges[edges_len++] = &v->edge[DIR_STRAIGHT];
            // }
            // if (v->edge[DIR_CURVED].owner == -1 || v->edge[DIR_CURVED].owner == owner) {
            //   edges[edges_len++] = &v->edge[DIR_CURVED];
            // }
            edges[edges_len++] = &v->edge[DIR_STRAIGHT];
            edges[edges_len++] = &v->edge[DIR_CURVED];
            break;
          default:
            KASSERT(false, "Node type not handled. node_id=%d type=%d. src=%d dest=%d", v->id, v->type, src, dest);
            break;
        }
        for (j = 0; j < edges_len; j++) {
            track_edge *e = edges[j];
            track_node *u = e->dest;
            if (!u->visited[p_idx] && v->p_dist[p_idx] + e->dist <= u->p_dist[p_idx]) {
                u->prev[p_idx] = i;
                u->p_dist[p_idx] = v->p_dist[p_idx] + e->dist;
                heap_push(h, u->p_dist[p_idx], (void *) u->id);
            }
        }
    }
}

int get_path(int src, int dest, track_node **path, int path_buf_size, int p_idx) {
  int n;
  int i;
  track_node *v, *u;
  v = &track[dest];

  if (v->p_dist[p_idx] == INT_MAX) {
    return -1;
  }

  // Get n, the length of the path
  for (n = 1, u = v; u->p_dist[p_idx]; u = &track[u->prev[p_idx]], n++);
  KASSERT(path_buf_size >= n, "Path buffer wasn't large enough. Would overflowing. src=%s dest=%s dist=%d n=%d", track[src].name, track[dest].name, track[dest].p_dist[p_idx], n);

  // follow the path backwards
  path[n - 1] = &track[dest];
  for (i = 0, u = v; u->p_dist[p_idx]; u = &track[u->prev[p_idx]], i++) {
    path[n - i - 2] = &track[u->prev[p_idx]];
  }
  return n;
}
