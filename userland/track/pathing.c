#include <basic.h>
#include <heap.h>
#include <jstring.h>
#include <track/pathing.h>
#include <track/pathing.h>

bool pathing_initialized = false;

track_node track[TRACK_MAX];

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

void GetPath(path_t *p, int src, int dest) {
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


void dijkstra(int src, int dest) {
    KASSERT(src >= 0 && dest >= 0, "Bad src or dest: got src=%d dest=%d", src, dest);
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
            KASSERT(false, "Node type not handled. node_id=%d type=%d. src=%d dest=%d", v->id, v->type, src, dest);
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
