#include <io.h>
#include <basic.h>
#include <util.h>
#include <bwio.h>
#include <packet.h>
#include <interactive.h>
#include <servers/nameserver.h>
#include <servers/clock_server.h>
#include <terminal.h>
#include <servers/uart_rx_server.h>
#include <servers/uart_tx_server.h>
#include <train_command_server.h>
#include <kernel.h>
#include <trains/navigation.h>
#include <trains/sensor_collector.h>
#include <trains/train_controller.h>
#include <detective/interval_detector.h>
#include <trains/switch_controller.h>
#include <jstring.h>
#include <priorities.h>
#include <interactive/command_parser.h>
#include <interactive/interactive.h>
#include <track/track_node.h>
#include <track/pathing.h>

// used for displaying the path, updated on 100ms intervals
path_t *current_path;
bool is_pathing;
int pathing_start_time;
bool clear_path_display;

// used for pathing -- the train properties and whether to stop, and where to
int velocity_reading_delay_until;
int most_recent_sensor;

// TODO: make time a string format type
void PrintTicks(int ticks) {
  char buf[32];
  KASSERT(ticks < 360000, "Uptime exceeded an hour. Can't handle time display, ticks (%d) >= 360000", ticks);
  int minutes = (ticks / 6000) % 24;
  int seconds = (ticks / 100) % 60;
  jformatf(buf, 32, "%02d:%02d %02d0", minutes, seconds, ticks % 100);
  Putstr(COM2, buf);

  Logs(UPTIME_LOGGING, buf);
}

#define PATH_LOG_X 60
#define PATH_LOG_Y 2

int path_display_pos;

void DisplayPath(path_t *p, int train, int speed, int start_time, int curr_time) {
  Putstr(COM2, SAVE_CURSOR);
  int i;
  for (i = 0; i < path_display_pos + 1; i++) {
    MoveTerminalCursor(PATH_LOG_X, PATH_LOG_Y + i);
    Putstr(COM2, CLEAR_LINE_AFTER);
  }

  for (i = 0; i < p->len; i++) {
    p->nodes[i]->actual_sensor_trip = -1;
  }

  path_display_pos = 0;

  int stop_dist = train == -2 ? 0 : StoppingDistance(train, speed);
  int velo = train == -2 ? 0 : Velocity(train, speed);

  // amount of mm travelled at this speed for the amount of time passed
  int travelled_dist = ((curr_time - start_time) * velo) / 100;

  int dist_to_dest = p->dist;
  int remaining_mm = dist_to_dest - stop_dist - travelled_dist;
  int calculated_time = remaining_mm * 10 / velo;
  if (calculated_time < 0) calculated_time = 0;


  MoveTerminalCursor(PATH_LOG_X, PATH_LOG_Y + path_display_pos);
  Putf(COM2, "Path from %s ~> %s", p->src->name, p->dest->name);
  MoveTerminalCursor(100, PATH_LOG_Y + path_display_pos);
  Putf(COM2, "dist %5dmm", remaining_mm + stop_dist);
  // only show ETA for navigation
  if (train != -2) {
    Putstr(COM2, "   timeleft=");
    PrintTicks(calculated_time);
  }
  int dist_sum = 0;

  // don't start at the first because it's easier
  for (i = 1; i < p->len; i++) {
    track_edge *next_edge;
    // if branch, figure out which way
    // also find the edge to get the true distance (node.dist is incorrect for merged paths, i.e. SRC -> BASIS -> DEST)
    char dir = 'x';
    if (p->nodes[i-1]->type == NODE_BRANCH) {
      if (p->nodes[i-1]->edge[DIR_CURVED].dest == p->nodes[i]) {
        dir = 'C';
        next_edge = &p->nodes[i-1]->edge[DIR_CURVED];
      } else {
        dir = 'S';
        next_edge = &p->nodes[i-1]->edge[DIR_STRAIGHT];
      }
    } else {
      next_edge = &p->nodes[i-1]->edge[DIR_AHEAD];
    }

    dist_sum += next_edge->dist;

    int remaining_mm_to_node;
    if (i == p->len - 1) {
      remaining_mm_to_node = dist_sum - stop_dist - travelled_dist;
    } else {
      remaining_mm_to_node = dist_sum - travelled_dist;
    }
    int eta_to_node = remaining_mm_to_node * 10 / velo;
    if (eta_to_node < 0) eta_to_node = 0;
    p->nodes[i]->expected_time = eta_to_node + start_time;

    if (p->nodes[i-1]->type == NODE_BRANCH) {
      path_display_pos++;
      MoveTerminalCursor(PATH_LOG_X + 4, PATH_LOG_Y + path_display_pos);
      Putf(COM2, "switch=%d needs to be %c", p->nodes[i-1]->num, dir);
    }

    path_display_pos++;
    MoveTerminalCursor(PATH_LOG_X + 2, PATH_LOG_Y + path_display_pos);
    Putf(COM2, "node=%s", p->nodes[i]->name);


    // print distance to individual node and time to it
    MoveTerminalCursor(100, PATH_LOG_Y + path_display_pos);
    Putf(COM2, "dist %5dmm", dist_sum);
    // only show ETA for navigating
    if (train != -2) {
      Putstr(COM2, "   eta=");
      PrintTicks(p->nodes[i]->expected_time);
    }
  }

  Putstr(COM2, RECOVER_CURSOR);
}

void UpdateDisplayPath(path_t *p, int train, int speed, int start_time, int curr_time) {
  Putstr(COM2, HIDE_CURSOR);
  int i;
  path_display_pos = 0;

  int stop_dist = StoppingDistance(train, speed);
  int velo = Velocity(train, speed);

  // amount of mm travelled at this speed for the amount of time passed
  int travelled_dist = ((curr_time - start_time) * velo) / 100;

  int dist_to_dest = p->dist;
  int remaining_mm = dist_to_dest - stop_dist - travelled_dist;
  int calculated_time = remaining_mm * 10 / velo;
  if (calculated_time < 0) calculated_time = 0;
  if (remaining_mm < 0) remaining_mm = -stop_dist;

  MoveTerminalCursor(100, PATH_LOG_Y + path_display_pos);
  Putf(COM2, "dist %5dmm   timeleft=", remaining_mm + stop_dist);
  PrintTicks(calculated_time);
  int dist_sum = 0;

  // don't start at the first because it's easier
  for (i = 1; i < p->len; i++) {
    track_edge *next_edge;
    // if branch, figure out which way
    // also find the edge to get the true distance (node.dist is incorrect for merged paths, i.e. SRC -> BASIS -> DEST)
    char dir = 'x';
    if (p->nodes[i-1]->type == NODE_BRANCH) {
      if (p->nodes[i-1]->edge[DIR_CURVED].dest == p->nodes[i]) {
        dir = 'C';
        next_edge = &p->nodes[i-1]->edge[DIR_CURVED];
      } else {
        dir = 'S';
        next_edge = &p->nodes[i-1]->edge[DIR_STRAIGHT];
      }
    } else {
      next_edge = &p->nodes[i-1]->edge[DIR_AHEAD];
    }

    dist_sum += next_edge->dist;

    int remaining_mm_to_node = dist_sum - stop_dist - travelled_dist;
    if (remaining_mm_to_node < 0) remaining_mm_to_node = -stop_dist;

    if (p->nodes[i-1]->type == NODE_BRANCH) {
      path_display_pos++;
    }

    path_display_pos++;

    // print distance to individual node and time to it
    MoveTerminalCursor(100, PATH_LOG_Y + path_display_pos);
    Putf(COM2, "dist %5dmm   eta=", remaining_mm_to_node + stop_dist);
    // only show ETA for navigating
    PrintTicks(p->nodes[i]->expected_time);
    if (p->nodes[i]->type == NODE_SENSOR && p->nodes[i]->actual_sensor_trip != -1) {
      Putstr(COM2, "  actual=");
      PrintTicks(p->nodes[i]->actual_sensor_trip);
    }
  }

  Putstr(COM2, SHOW_CURSOR);
}

void ClearLastCmdMessage() {
  Putstr(COM2, SAVE_CURSOR);
  MoveTerminalCursor(40, COMMAND_LOCATION + 1);
  Putstr(COM2, CLEAR_LINE RECOVER_CURSOR);
}

#define SENSOR_LOG_LENGTH 12

int sensor_display_nums[SENSOR_LOG_LENGTH];
int sensor_display_times[SENSOR_LOG_LENGTH];
int logged_sensors;
int last_logged_sensors;


void PrintSensorTrigger(int sensor_num, int sensor_time) {
  Putstr(COM2, SAVE_CURSOR);
  if (logged_sensors < SENSOR_LOG_LENGTH) logged_sensors++;
  last_logged_sensors = (last_logged_sensors + 1) % SENSOR_LOG_LENGTH;
  sensor_display_nums[last_logged_sensors] = sensor_num;
  sensor_display_times[last_logged_sensors] = sensor_time;
  int i;
  for (i = 0; i < logged_sensors; i++) {
    MoveTerminalCursor(30, SENSOR_HISTORY_LOCATION + 1 + i);
    Putstr(COM2, CLEAR_LINE_BEFORE);
    MoveTerminalCursor(0, SENSOR_HISTORY_LOCATION + 1 + i);
    if (last_logged_sensors == i) {
      Putstr(COM2, "=> ");
    } else {
      Putstr(COM2, "   ");
    }
    Putf(COM2, "%3s", track[sensor_display_nums[i]].name);
    MoveTerminalCursor(8, SENSOR_HISTORY_LOCATION + 1 + i);
    PrintTicks(sensor_display_times[i]);
  }
  Putstr(COM2, RECOVER_CURSOR);
}

/**
 * This is for manually triggering sensors
 */
void TriggerSensor(int sensor_no, int sensor_time) {
  int sensor_detector_multiplexer_tid = WhoIsEnsured(NS_SENSOR_DETECTOR_MULTIPLEXER);
  int sensor_attributer_tid = WhoIsEnsured(NS_SENSOR_ATTRIBUTER);

  track[sensor_no].actual_sensor_trip = sensor_time;

  sensor_data_t req;
  req.packet.type = SENSOR_DATA;
  req.timestamp = Time();
  req.sensor_no = sensor_no;
  // Send to attributer
  SendSN(sensor_attributer_tid, req);
  // Send to detector multiplexer
  SendSN(sensor_detector_multiplexer_tid, req);

  // Display to UI
  // PrintSensorTrigger(sensor_no, sensor_time);
}

// void sensor_timeout() {
//   while (true) {
//     Delay(5);
//     Send(parent);
//   }
// }
//
// void sensor_query() {
//   Create(sensor_timeout);
//   requery = true;
//   while (true) {
//
//     if (requery) {
//       SendQuerySensor();
//       requery = false;
//       last_query_time = Time();
//     }
//
//     req = Recv(sender);
//
//     switch (req.type) {
//       case TIMER_REQUERY:
//         curr_time = Time();
//         if (curr_time - last_query_time >= 10) {
//           requery = true;
//         }
//         break;
//       case DATA:
//         update = compute_update(data);
//         Send(interactive, update);
//         requery = true;
//         break;
//     }
//   }
// }

void DrawInitialScreen() {
  Putstr(COM2, CLEAR_SCREEN);
  Putstr(COM2, HOME_CURSOR);
  Putstr(COM2, "Time xx:xx xx0\n\r");
  Putstr(COM2, "Idle xx.x%\n\r");

  // Change BG colour
  Putstr(COM2, BLACK_FG);
  Putstr(COM2, WHITE_BG);
  MoveTerminalCursor(0, SWITCH_LOCATION);
  Putstr(COM2, "Switch status");

  Putstr(COM2, RESET_ATTRIBUTES);
  Putstr(COM2, "\n\r1 ?    7 ?    13?    153?\n\r"
             "2 ?    8 ?    14?    154?\n\r"
             "3 ?    9 ?    15?    155?\n\r"
             "4 ?    10?    16?    156?\n\r"
             "5 ?    11?    17?\n\r"
             "6 ?    12?    18?");

  Putstr(COM2, BLACK_FG);
  Putstr(COM2, WHITE_BG);
  MoveTerminalCursor(0, SENSOR_HISTORY_LOCATION);
  Putstr(COM2, "Recent sensor triggers");

  MoveTerminalCursor(0, COMMAND_LOCATION);
  Putstr(COM2, "Commands");
  // Reset BG colour
  Putstr(COM2, RESET_ATTRIBUTES);

  Putstr(COM2, "\n\rPlease wait, initializing...\n\r# ");

}

void DrawTime(int t) {
  // "Time 10:22 100"
  //       ^ 6th char
  MoveTerminalCursor(6, 0);
  PrintTicks(t);
}

io_time_t last_time_idle_displayed;
io_time_t idle_execution_time;

void DrawIdlePercent() {
  // FIXME: is there a way to drop the use of kernel global variables here?

  // get idle total, current time, and total time for idle total start

  io_time_t last_idle_execution_time = idle_execution_time;
  idle_execution_time = GetIdleTaskExecutionTime();
  io_time_t idle_diff = idle_execution_time - last_idle_execution_time;
  io_time_t curr_time = io_get_time();
  io_time_t time_total = curr_time - last_time_idle_displayed;
  last_time_idle_displayed = curr_time;

  int idle_percent = (idle_diff * 1000) / time_total;

  // "Idle 99.9% "
  //       ^ 6th char
  MoveTerminalCursor(0, 2);

  Putstr(COM2, "Idle ");
  char buf[12];
  jformatf(buf, 12, "%02d.%01d%%", (idle_percent / 10) % 100, idle_percent % 10);
  Putstr(COM2, buf);
  Logs(IDLE_LOGGING, buf);
}

void RenderSwitchChange(int sw, int state) {
  int index = sw;
  if (index >= 153 && index <= 156) {
    index -= 134; // 153 -> 19, etc
  }
  index--;

  if (index >= 0 && index < NUM_SWITCHES) {
    Putstr(COM2, SAVE_CURSOR);
    MoveTerminalCursor(3+7*(index/6)+(index>=18?1:0), SWITCH_LOCATION+1+index%6);
    if (state == SWITCH_CURVED) {
      Putc(COM2, 'C');
    } else if (state == SWITCH_STRAIGHT) {
      Putc(COM2, 'S');
    } else {
      Putc(COM2, '?');
    }
    Putstr(COM2, RECOVER_CURSOR);
  }
}

void initSwitches(int *initSwitches) {
  for (int i = 0; i < NUM_SWITCHES; i++) {
    int switchNumber = i+1;
    if (switchNumber >= 19) {
      switchNumber += 134; // 19 -> 153, etc
    }
    RenderSwitchChange(switchNumber, initSwitches[i]);
    SetSwitch(switchNumber, initSwitches[i]);
    Delay(6);
  }
}

#define BUCKETS 10
#define SAMPLES 3

const int bucketSensors[BUCKETS] = {
  16*(3-1)+(14-1), // C14
  16*(1-1)+( 4-1), // A 4
  16*(2-1)+(16-1), // B16
  16*(3-1)+( 5-1), // C 5
  16*(3-1)+(15-1), // C15
  16*(4-1)+(12-1), // D12
  16*(5-1)+(11-1), // E11
  16*(4-1)+(10-1), // D10
  16*(4-1)+( 8-1), // D 8
  16*(5-1)+( 8-1)  // E 8
};

const int sensorDistances[BUCKETS] = {
  785,
  589,
  440,
  485,
  293,
  404,
  284,
  277,
  774,
  375
};

int bucketSamples[BUCKETS*SAMPLES];
int bucketSize[BUCKETS];
float bucketAvg[BUCKETS];
float speedMultipliers[BUCKETS*SAMPLES];
int speedMultSize[BUCKETS];
int lastTrain = 58;

float predictions[BUCKETS*SAMPLES];
int predictionSize[BUCKETS];

int count = 0;
float prediction = 0;
float offset = 0;
float predictionAccuracy = 0;

void clearBuckets() {
  count = 0;
  for (int i = 0; i < BUCKETS; i++) {
    bucketSize[i] = 0;
    speedMultSize[i] = 0;
    predictionSize[i] = 0;
    prediction = 0.0f;
    for (int j = 0; j < SAMPLES; j++) {
      bucketSamples[i*SAMPLES+j] = 0;
      speedMultipliers[i*SAMPLES+j] = 1.0f;
      predictions[i*SAMPLES+j] = 1.0f;
    }
  }
}

void registerSample(int sensor, int prevSensor, int sample, int time) {
  for (int i = 0; i < BUCKETS; i++) {
    if (bucketSensors[i] == sensor) {
      if (bucketSize[i] < SAMPLES) {
        int j = (i == 0 ? BUCKETS : i) - 1;
        if (bucketSensors[j] == prevSensor) {
          bucketSamples[i*SAMPLES+bucketSize[i]] = sample;
          bucketSize[i]++;
        }
      }
      if (count > 1) {
        int total = 0;
        for (int j = 0; j < bucketSize[i]; j++) {
          total += bucketSamples[i*SAMPLES+j];
        }
        bucketAvg[i] = total/(float)bucketSize[i];
        speedMultipliers[i*SAMPLES+speedMultSize[i]] = ((float)sample)/(float)bucketAvg[i];
        speedMultSize[i]++;
        int sampleWithLastOffset = sample - offset;
        int predictedSample = prediction - (time-sampleWithLastOffset);
        offset = ((float)sample - predictedSample);
        float maxO = 5.0f;
        offset = minf(maxf(offset, -maxO), maxO);
        if (prediction != 0.0f) {
          predictionAccuracy = ((float)predictedSample) / ((float)sample);
        }
        prediction = (float)time + bucketAvg[(i+1)%BUCKETS] + offset;
      }
      // if (i == 7) {
      //   if (count > 3) {
      //     //DelayUntil(prediction-2);
      //     //clearBuckets();
      //     //DelayUntil((int)((float)time + bucketAvg[(i+1)%BUCKETS] + minf(maxf(offset, -20.0f), 20.0f)));
      //     DelayUntil((int)((float)time + bucketAvg[(i+1)%BUCKETS] + offset));
      //     SetTrainSpeed(lastTrain, 0);
      //   }
      //   count++;
      // }
      break;
    }
  }
}

int lastSensor = -1;

int sensor_reading_timestamps[256];

int sample_points[1000];
int samples = 0;

#define NUM_SENSORS 80

void stopper() {
  int sender;
  int train;
  int delay;
  while (true) {
    Receive(&sender, &train, sizeof(int));
    ReplyN(sender);
    Receive(&sender, &delay, sizeof(int));
    ReplyN(sender);
    Delay(delay);
    SetTrainSpeed(train, 0);
    velocity_reading_delay_until = Time();
    is_pathing = false;
    clear_path_display = true;
  }
}
//
// void sensor_saver() {
//   int stopper_tid = Create(2, stopper);
//   RegisterAs(NS_SENSOR_SAVER);
//   char request_buffer[1024] __attribute__ ((aligned (4)));
//   packet_t *packet = (packet_t *) request_buffer;
//   int lastSensorTime = -1;
//   int sender;
//
//   int C14 = Name2Node("C14");
//   int D12 = Name2Node("D12");
//   int C8 = Name2Node("C8");
//   int C15 = Name2Node("C15");
//   int E8 = Name2Node("E8");
//   int C10 = Name2Node("C10");
//   int B1 = Name2Node("B1");
//   int E14 = Name2Node("E14");
//
//   int prevSensor[NUM_SENSORS][2];
//   int sensorDistances[NUM_SENSORS][2];
//
//   for (int i = 0; i < 80; i++) {
//     prevSensor[i][0] = -1;
//     prevSensor[i][1] = -1;
//   }
//
//   for (int i = 0; i < 80; i++) {
//     int next1 = -1;
//     int next2 = -1;
//
//     int next = findSensorOrBranch(i);
//     if (track[next].type == NODE_BRANCH) {
//       next1 = track[next].edge[DIR_STRAIGHT].dest->id;
//       next2 = track[next].edge[DIR_CURVED].dest->id;
//       if (track[next1].type != NODE_SENSOR) {
//         next1 = findSensorOrBranch(next1);
//       }
//       if (track[next2].type != NODE_SENSOR) {
//         next2 = findSensorOrBranch(next2);
//       }
//       //KASSERT(next1 >= -1 && next1 < NUM_SENSORS, "next1 broken, got %d, started at %d, intermediary %d", next1, i, next);
//       //KASSERT(next2 >= -1 && next2 < NUM_SENSORS, "next2 broken, got %d, started at %d, intermediary %d", next2, i, next);
//     } else {
//       next1 = next;
//       //KASSERT(next1 >= -1 && next1 < NUM_SENSORS, "next1 broken, got %d", next1);
//     }
//
//     if (next1 >= 0 && next1 < NUM_SENSORS) {
//       for (int j = 0; j < 2; j++) {
//         if (prevSensor[next1][j] == -1) {
//           prevSensor[next1][j] = i;
//         }
//       }
//     }
//     if (next2 >= 0 && next2 < NUM_SENSORS) {
//       for (int j = 0; j < 2; j++) {
//         if (prevSensor[next2][j] == -1) {
//           prevSensor[next2][j] = i;
//         }
//       }
//     }
//   }
//
//   path_t p;
//   for (int i = 0; i < 80; i++) {
//     for (int j = 0; j < 2; j++) {
//       if (prevSensor[i][j] != -1) {
//         GetPath(&p, prevSensor[i][j], i);
//         sensorDistances[i][j] = p.dist;
//       }
//     }
//   }
//
//   GetPath(&p, Name2Node("E8"), Name2Node("C14"));
//   int E8_C14_dist = p.dist;
//
//   GetPath(&p, Name2Node("C15"), Name2Node("D12"));
//   int C15_D12_dist = p.dist;
//
//   GetPath(&p, Name2Node("E14"), Name2Node("E8"));
//   int E14_E8_dist = p.dist;
//
//   GetPath(&p, Name2Node("B1"), Name2Node("E14"));
//   int B1_E14_dist = p.dist;
//
//   GetPath(&p, Name2Node("E14"), Name2Node("E9"));
//   int E14_E9_dist = p.dist;
//
//   GetPath(&p, Name2Node("E14"), Name2Node("C14"));
//   int E14_C14_dist = p.dist;
//
//   GetPath(&p, Name2Node("C14"), Name2Node("C10"));
//   int C14_C10_dist = p.dist;
//
//   DECLARE_BASIS_NODE(basis_node);
//
//   while (true) {
//     ReceiveS(&sender, request_buffer);
//     switch (packet->type) {
//     case SENSOR_DATA: {
//           sensor_data_t * data = (sensor_data_t *) request_buffer;
//           int curr_time = Time();
//           sensor_reading_timestamps[data->sensor_no] = curr_time;
//           TriggerSensor(data->sensor_no, curr_time);
//
//           int velocity = 0;
//           if (lastSensor != -1) {
//             for (int i = 0; i < 2; i++) {
//               if (prevSensor[data->sensor_no][i] == lastSensor) {
//                 int time_diff = sensor_reading_timestamps[data->sensor_no] - sensor_reading_timestamps[lastSensor];
//                 velocity = (sensorDistances[data->sensor_no][i] * 100) / time_diff;
//                 // RecordLogf("Readings for %2d ~> %2d : time_diff=%5d velocity=%3dmm/s (curve)\n\r", prevSensor[data->sensor_no][i], data->sensor_no, time_diff*10, velocity);
//               }
//             }
//           }
//           if (velocity > 0 && (curr_time - velocity_reading_delay_until) > 400) {
//             record_velocity_sample(active_train, active_speed, velocity);
//           }
//
//           // int time = Time();
//           // int diffTime = time - lastSensorTime;
//           // if (lastSensor > 0) {
//           //   registerSample(data->sensor_no, lastSensor, diffTime, time);
//           // }
//           lastSensor = data->sensor_no;
//           lastSensorTime = curr_time;
//         break;
//     } default:
//       KASSERT(false, "Received unknown request type.");
//     }
//     ReplyN(sender);
//   }
// }

void interactive() {
  path_t p;
  logged_sensors = 0;
  last_logged_sensors = SENSOR_LOG_LENGTH - 1; // goes around
  current_path = &p;
  velocity_reading_delay_until = 0;
  path_display_pos = 0;
  samples = 0;
  is_pathing = false;
  clear_path_display = false;
  int path_update_counter = 0;

  int tid = MyTid();
  RegisterAs(NS_INTERACTIVE);

  // FIXME: priority
  int time_keeper_id = StartIntervalDetector("interactive time keeper", tid, 10);
  int sender;
  idle_execution_time = 0;
  last_time_idle_displayed = 0;

  DrawInitialScreen();
  Putstr(COM2, SAVE_CURSOR);
  DrawTime(Time());
  Putstr(COM2, RECOVER_CURSOR);

  Delay(25);

  int initialSwitchStates[NUM_SWITCHES];
  initialSwitchStates[ 0] = SWITCH_CURVED;
  initialSwitchStates[ 1] = SWITCH_CURVED;
  initialSwitchStates[ 2] = SWITCH_STRAIGHT;
  initialSwitchStates[ 3] = SWITCH_CURVED;
  initialSwitchStates[ 4] = SWITCH_CURVED;
  initialSwitchStates[ 5] = SWITCH_STRAIGHT;
  initialSwitchStates[ 6] = SWITCH_STRAIGHT;
  initialSwitchStates[ 7] = SWITCH_STRAIGHT;
  initialSwitchStates[ 8] = SWITCH_STRAIGHT;
  initialSwitchStates[ 9] = SWITCH_STRAIGHT;
  initialSwitchStates[10] = SWITCH_CURVED;
  initialSwitchStates[11] = SWITCH_STRAIGHT;
  initialSwitchStates[12] = SWITCH_STRAIGHT;
  initialSwitchStates[13] = SWITCH_CURVED;
  initialSwitchStates[14] = SWITCH_CURVED;
  initialSwitchStates[15] = SWITCH_STRAIGHT;
  initialSwitchStates[16] = SWITCH_STRAIGHT;
  initialSwitchStates[17] = SWITCH_CURVED;
  initialSwitchStates[18] = SWITCH_STRAIGHT;
  initialSwitchStates[19] = SWITCH_CURVED;
  initialSwitchStates[20] = SWITCH_STRAIGHT;
  initialSwitchStates[21] = SWITCH_CURVED;
  initSwitches(initialSwitchStates);

  char request_buffer[1024] __attribute__ ((aligned (4)));
  packet_t *packet = (packet_t *) request_buffer;
  interactive_echo_t *echo_data = (interactive_echo_t *) request_buffer;
  cmd_t *base_cmd = (cmd_t *) request_buffer;
  cmd_error_t *cmd_error = (cmd_error_t *) request_buffer;
  cmd_data_t *cmd_data = (cmd_data_t *) request_buffer;

  int lastSensor = -1;
  int lastSensorTime = -1;
  clearBuckets();

  ClearLastCmdMessage();

  int trainControllerTids[TRAINS_MAX];
  for (int i = 0; i < TRAINS_MAX; i++) {
    trainControllerTids[i] = -1;
  }

  while (true) {
    ReceiveS(&sender, request_buffer);
    switch (packet->type) {
      case INTERACTIVE_ECHO:
        Putstr(COM2, echo_data->echo);
        break;
      case INTERVAL_DETECT:
        Putstr(COM2, SAVE_CURSOR);
        int cur_time = Time();
        DrawTime(cur_time);
        DrawIdlePercent();

        // if (is_pathing && path_update_counter >= 3) {
        //   path_update_counter = 0;
        //   UpdateDisplayPath(current_path, active_train, active_speed, pathing_start_time, cur_time);
        // } else {
        //   path_update_counter++;
        // }
        Putstr(COM2, RECOVER_CURSOR);
        break;
      case INTERPRETED_COMMAND:
        ClearLastCmdMessage();
        MoveTerminalCursor(0, COMMAND_LOCATION + 1);
        switch (base_cmd->type) {
        case COMMAND_INVALID:
          Putstr(COM2, cmd_error->error);
          break;
        case COMMAND_TRAIN_SPEED:
          Putf(COM2, "Set train %d to speed %d", cmd_data->train, cmd_data->speed);
          RecordLogf("Set train %d to speed %d\n\r", cmd_data->train, cmd_data->speed);
          samples = 0;
          lastTrain = cmd_data->train;
          velocity_reading_delay_until = Time();
          break;
        case COMMAND_TRAIN_REVERSE:
          Putf(COM2, "Train %d reverse", cmd_data->train);
          break;
        case COMMAND_SWITCH_TOGGLE:
          RenderSwitchChange(cmd_data->switch_no, cmd_data->switch_dir);
          Putf(COM2, "Set switch %d to %c", cmd_data->switch_no, cmd_data->switch_dir == DIR_STRAIGHT ? 'S' : 'C');
          break;
        case COMMAND_SWITCH_TOGGLE_ALL:
          Putf(COM2, "Set all switches to %c", cmd_data->switch_dir == DIR_STRAIGHT ? 'S' : 'C');
          for (int i = 0; i < NUM_SWITCHES; i++) {
            int switchNumber = i+1;
            if (switchNumber >= 19) {
              switchNumber += 134; // 19 -> 153, etc
            }
            RenderSwitchChange(switchNumber, cmd_data->switch_dir);
          }
          break;
        case COMMAND_QUIT:
          ExitKernel();
          break;
        case COMMAND_CLEAR_SENSOR_SAMPLES:
          clearBuckets();
          break;
        case COMMAND_CLEAR_SENSOR_OFFSET:
          prediction = 0.0f;
          offset = 0;
          lastSensor = -1;
          break;
        case COMMAND_PRINT_SENSOR_SAMPLES: {
          Putstr(COM2, SAVE_CURSOR);
          char buf[10];
          int speedTotal = 0;
          int n = 0;
          for (int i = 0; i < BUCKETS; i++) {
            MoveTerminalCursor(0, COMMAND_LOCATION + 5 + i);
            Putstr(COM2, CLEAR_LINE);
            int total = 0;
            for (int j = 0; j < bucketSize[i]; j++) {
              total += bucketSamples[i*SAMPLES+j];
            }
            int avg = (sensorDistances[i]*1000)/(total/bucketSize[i]);
            if (avg > 0) {
              speedTotal += avg;
              n++;
            }
            Putf(COM2, "%d  -  %d", total/bucketSize[i], (int)(avg*1000));

            //for (int j = 0; j < bucketSize[i]; j++) {
            //  MoveTerminalCursor((j+1) * 6, COMMAND_LOCATION + 3 + i);
            //  char buf[10];
            //  ji2a(bucketSamples[i*SAMPLES+j], buf);
            //  Putstr(COM2, buf);
            //}
          }
          MoveTerminalCursor(0, COMMAND_LOCATION + 3);
          Putf(COM2, "%d" RECOVER_CURSOR, speedTotal / n);
          break;
        }
        case COMMAND_PRINT_SENSOR_MULTIPLIERS: {
          Putstr(COM2, SAVE_CURSOR);
          char buf[10];
          for (int i = 0; i < BUCKETS; i++) {
            MoveTerminalCursor(0, COMMAND_LOCATION + 5 + i);
            Putstr(COM2, CLEAR_LINE);
            float total = 0;
            for (int j = 0; j < speedMultSize[i]; j++) {
              total += speedMultipliers[i*SAMPLES+j];
            }
            float avg = total/speedMultSize[i];
            Puti(COM2, (int)(avg*1000));
          }
          Putstr(COM2, RECOVER_CURSOR);
          break;
        }
        case COMMAND_PATH:
          GetPath(&p, cmd_data->src_node, cmd_data->dest_node);
          DisplayPath(&p, -2, -2, 0, 0);
          is_pathing = false;
          break;
        case COMMAND_NAVIGATE:
          if (WhereAmI(cmd_data->train) == -1) {
            Putf(COM2, "Train must have a current location to navigate.");
            break;
          }
          Putf(COM2, "Navigating train %2d from %4s ~> %4s.", cmd_data->train, track[WhereAmI(cmd_data->train)].name, track[cmd_data->dest_node].name);
          // active_train = cmd_data->train;
          // FIXME: navigate no longer has a speed, so this is hardcoded
          // active_speed = cmd_data->speed;
          // velocity_reading_delay_until = Time();

          // get the full path including BASIS_NODE and display it
          // GetMultiDestinationPath(&p, WhereAmI(cmd_data->train), BASIS_NODE_NAME, cmd_data->dest_node);
          // DisplayPath(&p, active_train, active_speed, 0, 0);
          // is_pathing = true;
          // pathing_start_time = Time();
          // // set the trains destination, this makes the pathing logic fire
          // // up when the train hits BASIS_NODE
          // stop_on_node = cmd_data->dest_node;
          // set_to_stop = true;
          break;
        case COMMAND_STOP_FROM:
          if (WhereAmI(cmd_data->train) == -1) {
            Putf(COM2, "Train must already have hit a sensor to path.");
            break;
          }
          Putf(COM2, "Moving train %2d from %4s ~> %4s to stop on %4s.", cmd_data->train, track[cmd_data->src_node].name, track[cmd_data->dest_node].name, track[cmd_data->dest_node].name);
          // // get the path to the stopping from node
          // GetPath(&p, WhereAmI(cmd_data->train), BASIS_NODE_NAME);
          // // display the path
          // DisplayPath(&p, active_train, active_speed, 0, 0);
          // is_pathing = true;
          // pathing_start_time = Time();
          break;
        case COMMAND_SET_VELOCITY:
          Putf(COM2, "Set velocity train=%d speed=%d to %dmm/s", cmd_data->train, cmd_data->speed, cmd_data->extra_arg);
          set_velocity(cmd_data->train, cmd_data->speed, cmd_data->extra_arg);
          break;
        case COMMAND_PRINT_VELOCITY:
          // Putf(COM2, "velocity=%dmm/s", Velocity(active_train, active_speed));
          break;
        case COMMAND_SET_LOCATION:
          Putf(COM2, "Setting train=%d stopped location to node=%s", cmd_data->train, track[cmd_data->src_node].name);
          SetTrainLocation(cmd_data->train, cmd_data->src_node);
          break;
        case COMMAND_STOPPING_DISTANCE_OFFSET:
          Putf(COM2, "Offsetting stopping distance train=%d speed=%d to %dmm", cmd_data->train, cmd_data->speed, cmd_data->extra_arg);
          offset_stopping_distance(cmd_data->train, cmd_data->speed, cmd_data->extra_arg);
          break;
        case COMMAND_SET_STOPPING_DISTANCEN:
          // Both this and the negative stopping distance are normalized
        case COMMAND_SET_STOPPING_DISTANCE:
          Putf(COM2, "Set stopping distance train=%d speed=%d to %dmm", cmd_data->train, cmd_data->speed, cmd_data->extra_arg);
          set_stopping_distance(cmd_data->train, cmd_data->speed, cmd_data->extra_arg);
          break;
        case COMMAND_MANUAL_SENSE:
          Putf(COM2, "Manually triggering sensor %s", track[cmd_data->dest_node].name);
          TriggerSensor(cmd_data->dest_node, Time());
        }
        MoveTerminalCursor(40, COMMAND_LOCATION + 2);
        Putstr(COM2, CLEAR_LINE_BEFORE);
        MoveTerminalCursor(0, COMMAND_LOCATION + 2);
        Putstr(COM2, "# ");
        break;
      default:
        KASSERT(false, "Bad type received: got type=%d", packet->type);
        break;
    }
    // NOTE: this is after because if the command parser runs again
    // it can/will write over the global_command_buffer
    ReplyN(sender);

  }
}
