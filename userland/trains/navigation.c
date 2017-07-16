#include <basic.h>
#include <trains/navigation.h>
#include <track/track_node.h>
#include <servers/clock_server.h>
#include <bwio.h>
#include <servers/uart_tx_server.h>
#include <trains/switch_controller.h>
#include <train_command_server.h>

typedef struct TrainState {
  int train_locations[TRAINS_MAX];
} train_state_t;

train_state_t state;

int navigation_intialized = false;
int velocity[TRAINS_MAX][15];
int stopping_distance[TRAINS_MAX][15];

int velocitySamples[TRAINS_MAX][15][VELOCITY_SAMPLES_MAX];
int velocitySampleStart[TRAINS_MAX][15];

void InitNavigation() {
  int i;
  navigation_intialized = true;

  for (i = 0; i < TRAINS_MAX; i++) {
      int j;
      for (j = 0; j < 15; j++) {
        velocity[i][j] = -1;
        stopping_distance[i][j] = 0;

        velocitySampleStart[i][j] = 0;
        int k;
        for (k = 0; k < VELOCITY_SAMPLES_MAX; k++) {
          velocitySamples[i][j][k] = 0;
        }
      }
  }

  //// all units are millimetres
  //velocity[69][10] = 460; // ~accurate
  //velocity[69][11] = 530; // ~accurate, (cut a bit short, should remeasure)
  //velocity[69][12] = 577; // ~accurate, averaged 575 - 579, tending to 579 (didnt spend too long)
  //velocity[69][13] = 600; // ~accurate, averaged 590-610, didn't leave on for awhile
  //velocity[69][14] = 610; // ~accurate, averaged 590-620, didn't leave on for awhile

  stopping_distance[69][10] = 280 + 56 + 30; // reasonably accurate
  stopping_distance[71][10] = 700; // reasonably accurate
  stopping_distance[71][12] = 400; // reasonably accurate
  stopping_distance[71][14] = -140; // reasonably accurate

  for (i = 0; i < TRAINS_MAX; i++) {
    state.train_locations[i] = -1;
  }

  // NOTE: fixture location for testing purposes, for non-active train #10
  state.train_locations[70] = Name2Node("B6");
  state.train_locations[69] = Name2Node("C9");
  // calibrated and using as fixture
  velocity[70][5] = 240;
  stopping_distance[70][5] = 230;
}

void set_location(int train, int location) {
  state.train_locations[train] = location;
}

int WhereAmI(int train) {
  KASSERT(state.train_locations[train] >= 0, "Train had bad location. train=%d location=%d", train, state.train_locations[train]);
  return state.train_locations[train];
}

void SetPathSwitches(path_t *p) {
  int i;
  char dir;
  RecordLogf("SetPathSwitches %s ~> %s\n\r", p->src->name, p->dest->name);
  for (i = 0; i < p->len; i++) {
    if (i > 0 && p->nodes[i-1]->type == NODE_BRANCH) {
      if (p->nodes[i-1]->edge[DIR_CURVED].dest == p->nodes[i]) {
        dir = 'C';
        SetSwitch(p->nodes[i-1]->num, SWITCH_CURVED);
      } else {
        dir = 'S';
        SetSwitch(p->nodes[i-1]->num, SWITCH_STRAIGHT);
      }
      RecordLogf("  Setting switch %s to %c", p->nodes[i-1]->name, dir);
    }
  }
}

void Navigate(int train, int speed, int src, int dest, bool include_stop) {
  if (velocity[train][speed] == -1) {
    KASSERT(false, "Train speed / stopping distance not yet calibrated");
  }
  KASSERT(src >= 0 && dest >= 0, "Bad src or dest: got src=%d dest=%d", src, dest);

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
  Putf(COM2, "Setting train=%d speed=%d for remaining_time=%d\n\r", train, speed, remainingTime);
  SetTrainSpeed(train, speed);

  if (include_stop) {
    Delay((remainingTime / 10));
    MoveTerminalCursor(0, COMMAND_LOCATION + 6);
    Putf(COM2, "Stopping train=%d\n\r" RECOVER_CURSOR, train);
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
  // These stopping distance values are actually the offset from C10 -> E14
  // thus we take dist(C10->E14) and subtract the offset for the stopping
  // distance
  return 1056 - stopping_distance[train][speed];
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
  KASSERT(train >= 0 && train <= TRAINS_MAX, "Invalid train when getting velocity. Got %d", train);
  KASSERT(speed >= 0 && speed <= 14, "Invalid speed when getting velocity. Got %d", speed);
  return velocity[train][speed];
}

void record_velocity_sample(int train, int speed, int sample) {
  KASSERT(train >= 0 && train <= TRAINS_MAX, "Invalid train when recording sample. Got %d", train);
  KASSERT(speed >= 0 && speed <= 14, "Invalid speed when recording sample. Got %d", speed);
  int startIndex = velocitySampleStart[train][speed];
  KASSERT(startIndex >= 0 && startIndex < VELOCITY_SAMPLES_MAX, "This shouldn't happen.");
  velocitySamples[train][speed][startIndex] = sample;
  velocitySampleStart[train][speed] = (startIndex + 1) % VELOCITY_SAMPLES_MAX;

  int total = 0;
  int n = 0;
  for (int i = 0; i < VELOCITY_SAMPLES_MAX; i++) {
    if (velocitySamples[train][speed][i] > 0) {
      total += velocitySamples[train][speed][i];
      n++;
    }
  }
  if (n > 0) {
    velocity[train][speed] = total/n;
  }
}

void set_velocity(int train, int speed, int velo) {
  velocity[train][speed] = velo;
}

void set_stopping_distance(int train, int speed, int distance) {
  stopping_distance[train][speed] = distance;
}

void offset_stopping_distance(int train, int speed, int offset) {
  stopping_distance[train][speed] += offset;
}
