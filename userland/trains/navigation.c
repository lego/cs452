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

// void init_tracktest_route_table() {
//   // MR, BR => MR, BR
//   route_table[0][0] = NULL;
//   route_table[0][1] = NULL;
//   route_table[1][0] = NULL;
//   route_table[1][1] = NULL;
//
//   // MR, BR => EN1, EX1
//   route_table[0][2] = &track[1].edge[DIR_STRAIGHT];
//   route_table[0][3] = &track[1].edge[DIR_STRAIGHT];
//   route_table[1][2] = &track[1].edge[DIR_STRAIGHT];
//   route_table[1][3] = &track[1].edge[DIR_STRAIGHT];
//
//   // MR, BR => EN2, EX2
//   route_table[0][4] = &track[1].edge[DIR_CURVED];
//   route_table[0][5] = &track[1].edge[DIR_CURVED];
//   route_table[1][4] = &track[1].edge[DIR_CURVED];
//   route_table[1][5] = &track[1].edge[DIR_CURVED];
//
//   // MR, BR => EN3, EX3
//   route_table[0][6] = &track[0].edge[DIR_AHEAD];
//   route_table[0][7] = &track[0].edge[DIR_AHEAD];
//   route_table[1][6] = &track[0].edge[DIR_AHEAD];
//   route_table[1][7] = &track[0].edge[DIR_AHEAD];
//
//
//
//   // EN1, EX1 => EN1, EX1
//   route_table[2][2] = NULL;
//   route_table[2][3] = NULL;
//   route_table[3][2] = NULL;
//   route_table[3][3] = NULL;
//
//   // EN1, EX1 => *
//   route_table[2][0] = &track[2].edge[DIR_AHEAD];
//   route_table[2][1] = &track[2].edge[DIR_AHEAD];
//   route_table[3][0] = &track[2].edge[DIR_AHEAD];
//   route_table[3][1] = &track[2].edge[DIR_AHEAD];
//   route_table[2][4] = &track[2].edge[DIR_AHEAD];
//   route_table[2][5] = &track[2].edge[DIR_AHEAD];
//   route_table[3][4] = &track[2].edge[DIR_AHEAD];
//   route_table[3][5] = &track[2].edge[DIR_AHEAD];
//   route_table[2][6] = &track[2].edge[DIR_AHEAD];
//   route_table[2][7] = &track[2].edge[DIR_AHEAD];
//   route_table[3][6] = &track[2].edge[DIR_AHEAD];
//   route_table[3][7] = &track[2].edge[DIR_AHEAD];
//
//
//
//   // EN2, EX2 => EN2, EX2
//   route_table[4][4] = NULL;
//   route_table[4][5] = NULL;
//   route_table[5][4] = NULL;
//   route_table[5][5] = NULL;
//
//   // EN2, EX2 => 4
//   route_table[4][0] = &track[4].edge[DIR_AHEAD];
//   route_table[4][1] = &track[4].edge[DIR_AHEAD];
//   route_table[5][0] = &track[4].edge[DIR_AHEAD];
//   route_table[5][1] = &track[4].edge[DIR_AHEAD];
//   route_table[4][2] = &track[4].edge[DIR_AHEAD];
//   route_table[4][3] = &track[4].edge[DIR_AHEAD];
//   route_table[5][2] = &track[4].edge[DIR_AHEAD];
//   route_table[5][3] = &track[4].edge[DIR_AHEAD];
//   route_table[4][6] = &track[4].edge[DIR_AHEAD];
//   route_table[4][7] = &track[4].edge[DIR_AHEAD];
//   route_table[5][6] = &track[4].edge[DIR_AHEAD];
//   route_table[5][7] = &track[4].edge[DIR_AHEAD];
//
//
//   // EN3, EX3 => EN3, EX3
//   route_table[6][6] = NULL;
//   route_table[6][7] = NULL;
//   route_table[7][6] = NULL;
//   route_table[7][7] = NULL;
//
//   // EN3, EX3 => *
//   route_table[6][0] = &track[6].edge[DIR_AHEAD];
//   route_table[6][1] = &track[6].edge[DIR_AHEAD];
//   route_table[7][0] = &track[6].edge[DIR_AHEAD];
//   route_table[7][1] = &track[6].edge[DIR_AHEAD];
//   route_table[6][4] = &track[6].edge[DIR_AHEAD];
//   route_table[6][5] = &track[6].edge[DIR_AHEAD];
//   route_table[7][4] = &track[6].edge[DIR_AHEAD];
//   route_table[7][5] = &track[6].edge[DIR_AHEAD];
//   route_table[6][2] = &track[6].edge[DIR_AHEAD];
//   route_table[6][3] = &track[6].edge[DIR_AHEAD];
//   route_table[7][2] = &track[6].edge[DIR_AHEAD];
//   route_table[7][3] = &track[6].edge[DIR_AHEAD];
// }
//
// int minDistance(int dist[], bool sptSet[]) {
//    // Initialize min value
//    int min = INT_MAX, min_index;
//
//    for (int v = 0; v < V; v++)
//      if (sptSet[v] == false && dist[v] <= min)
//          min = dist[v], min_index = v;
//
//    return min_index;
// }
//
// // Funtion that implements Dijkstra's single source shortest path algorithm
// // for a graph represented using adjacency matrix representation
// void dijkstra(int graph[V][V], int src) {
//      int dist[V];     // The output array.  dist[i] will hold the shortest
//                       // distance from src to i
//
//      bool sptSet[V]; // sptSet[i] will true if vertex i is included in shortest
//                      // path tree or shortest distance from src to i is finalized
//
//      // Initialize all distances as INFINITE and stpSet[] as false
//      for (int i = 0; i < V; i++)
//         dist[i] = INT_MAX, sptSet[i] = false;
//
//      // Distance of source vertex from itself is always 0
//      dist[src] = 0;
//
//      // Find shortest path for all vertices
//      for (int count = 0; count < V-1; count++)
//      {
//        // Pick the minimum distance vertex from the set of vertices not
//        // yet processed. u is always equal to src in first iteration.
//        int u = minDistance(dist, sptSet);
//
//        // Mark the picked vertex as processed
//        sptSet[u] = true;
//
//        // Update dist value of the adjacent vertices of the picked vertex.
//        for (int v = 0; v < V; v++)
//
//          // Update dist[v] only if is not in sptSet, there is an edge from
//          // u to v, and total weight of path from src to  v through u is
//          // smaller than current value of dist[v]
//          if (!sptSet[v] && graph[u][v] && dist[u] != INT_MAX
//                                        && dist[u]+graph[u][v] < dist[v])
//             dist[v] = dist[u] + graph[u][v];
//      }
//
//      // print the constructed distance array
//      printSolution(dist, V);
// }
//
// int min_distance_node(int src, bool *unprocessed_nodes) {
//   int i;
//   int min = INT_MAX;
//   int min_idx;
//   for (i = 0; i < TRACK_MAX; i++) {
//     if (unprocessed_nodes[i]) {
//
//     }
//   }
// }
//
// void route_from_src(int src) {
//   int i;
//   int j;
//   track_node *src_node = &track[src];
//   track_node *rev_node = src_node->reverse;
//   int rev = rev_node->id;
//   bool processed_node[TRACK_MAX];
//   heapnode_t heap_nodes[TRACK_MAX+1];
//   heap_t processing = heap_create(&heap_nodes, TRACK_MAX+1);
//
//   for (i = 0; i < TRACK_MAX; i++) {
//     processed_node[i] = false;
//   }
//
//   // fill default routes for src
//   processed_node[src] = true;
//   processed_node[rev] = true;
//
//   track_edge *edge;
//
//   // start the routing for each node type
//   switch (node->type) {
//     case NODE_EXIT:
//       node = node->reverse;
//     case NODE_ENTER:
//       // add ahead
//       edge = node->edges[DIR_AHEAD];
//       heap_push(&processing, edge->dist, edge->dest);
//       break;
//     case NODE_SENSOR:
//       // add ahead
//       edge = node->edges[DIR_AHEAD];
//       heap_push(&processing, edge->dist, edge->dest);
//       // add reverse-ahead
//       edge = node->reverse->edges[DIR_AHEAD];
//       heap_push(&processing, edge->dist, edge->dest);
//       break;
//     case NODE_MERGE:
//       node = node->reverse;
//     case NODE_BRANCH:
//       // add curve
//       edge = node->edges[DIR_CURVE];
//       heap_push(&processing, edge->dist, edge->dest);
//       // add straight
//       edge = node->edges[DIR_STRAIGHT];
//       heap_push(&processing, edge->dist, edge->dest);
//       // add reverse-ahead
//       edge = node->reverse->edges[DIR_AHEAD];
//       heap_push(&processing, edge->dist, edge->dest);
//       break;
//     default:
//       KASSERT(false, "Unhandled node type: node_id=%d type=%d", node->id, node->type);
//   }
//
//   while(heap_size(&processing) > 0) {
//     int node_dist = heap_peek_priority(&processing);
//     track_node *node = heap_pop(&processing);
//     if (processed_node[node->id]) continue;
//
//     route_table_dist[src][node->id] = node_dist;
//
//     switch (node->type) {
//       case NODE_EXIT:
//         node = node->reverse;
//       case NODE_ENTER:
//         // add ahead
//         edge = node->edges[DIR_AHEAD];
//         heap_push(&processing, edge->dist, edge->dest);
//         break;
//       case NODE_SENSOR:
//         // add ahead
//         edge = node->edges[DIR_AHEAD];
//         heap_push(&processing, edge->dist, edge->dest);
//         // add reverse-ahead
//         edge = node->reverse->edges[DIR_AHEAD];
//         heap_push(&processing, edge->dist, edge->dest);
//         break;
//       case NODE_MERGE:
//         node = node->reverse;
//       case NODE_BRANCH:
//         // add curve
//         edge = node->edges[DIR_CURVE];
//         heap_push(&processing, edge->dist, edge->dest);
//         // add straight
//         edge = node->edges[DIR_STRAIGHT];
//         heap_push(&processing, edge->dist, edge->dest);
//         // add reverse-ahead
//         edge = node->reverse->edges[DIR_AHEAD];
//         heap_push(&processing, edge->dist, edge->dest);
//         break;
//       default:
//         KASSERT(false, "Unhandled node type: node_id=%d type=%d", node->id, node->type);
//     }
//
//     // add reverse, same cost as current node
//     heap_push(&processing, route_table_dist[src][node->id], node->reverse);
//   }
// }
//
// void init_route_table() {
//   int i;
//   int j;
//   for (i = 0; i < TRACK_MAX; i++) {
//     for (j = 0; j < TRACK_MAX; j++) {
//       route_table[i][j] = -1;
//       route_table_dist[i][j] = -1;
//     }
//   }
//
//   for (i = 0; i < TRACK_MAX; i++) {
//     route_table[i][i] = NULL;
//     route_table_dist[i][i] = 0;
//     track_node *node = track[i];
//     track_edge *edge;
//
//     switch (node->type) {
//       case NODE_EXIT:
//         node = node->reverse;
//       case NODE_ENTER:
//         edge = node->edges[DIR_AHEAD];
//         // add ahead, with current cost + edge
//         track_node *dest_node = edge->dest;
//         route_table[i][dest_node->id] = edge;
//         route_table_dist[i][edge->dest->id] = edge->dist;
//         break;
//       case NODE_SENSOR:
//         // add ahead
//         edge = node->edges[DIR_AHEAD];
//         track_node *dest_node = edge->dest;
//         route_table[i][dest_node->id] = edge;
//         route_table_dist[i][edge->dest->id] = edge->dist;
//
//         // add reverse ahead
//         edge = node->reverse->edges[DIR_AHEAD];
//         track_node *dest_node = edge->dest;
//         route_table[i][dest_node->id] = edge;
//         route_table_dist[i][edge->dest->id] = edge->dist;
//         break;
//       case NODE_MERGE:
//         node = node->reverse;
//       case NODE_BRANCH:
//         // add curve
//         edge = node->edges[DIR_STRAIGHT];
//         track_node *dest_node = edge->dest;
//         route_table[i][dest_node->id] = edge;
//         route_table_dist[i][edge->dest->id] = edge->dist;
//
//         // add straight
//         edge = node->edges[DIR_CURVE];
//         track_node *dest_node = edge->dest;
//         route_table[i][dest_node->id] = edge;
//         route_table_dist[i][edge->dest->id] = edge->dist;
//
//         // add reverse ahead
//         edge = node->reverse->edges[DIR_AHEAD];
//         track_node *dest_node = edge->dest;
//         route_table[i][dest_node->id] = edge;
//         route_table_dist[i][edge->dest->id] = edge->dist;
//         break;
//       default:
//         KASSERT(false, "Unhandled node type: node_id=%d type=%d", node->id, node->type);
//     }
//   }
//
//
//   for (i = 0; i < TRACK_MAX; i++) {
//     route_from_src(i);
//   }
// }

// FIXME: fixtures while testing
void ReverseTrainStub(int train) {
  bwprintf(COM2, "reversing train=%d\n\r", train);
}

void SetTrainSpeedStub(int train, int speed) {
  bwprintf(COM2, "setting speed=%d train=%d\n\r", speed, train);
}

void InitNavigation() {
  int i;

  #if defined(USE_TRACKA)
  init_tracka(track);
  #elif defined(USE_TRACKB)
  init_trackb(track);
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
