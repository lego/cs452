#include <basic.h>
#include <kernel.h>
#include <packet.h>
#include <servers/nameserver.h>
#include <servers/uart_tx_server.h>
#include <servers/clock_server.h>
#include <train_controller.h>
#include <trains/navigation.h>
#include <trains/sensor_saver.h>
#include <trains/sensor_collector.h>

extern bool is_pathing; // FIXME: global

// used for pathing -- the train properties and whether to stop, and where to
extern int active_train; // FIXME: global
extern int active_speed; // FIXME: global
extern int velocity_reading_delay_until; // FIXME: global
extern bool set_to_stop; // FIXME: global
extern bool set_to_stop_from; // FIXME: global
extern int stop_on_node; // FIXME: global

int lastSensor = -1;
int sensor_reading_timestamps[256];

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

#define NUM_SENSORS 80

// FIXME: this should instead message interative to do the printing
void TriggerSensor(int sensor_num, int sensor_time) {
  track[sensor_num].actual_sensor_trip = sensor_time;
  PrintSensorTrigger(sensor_num, sensor_time);
}

int findSensorOrBranch(int start) {
  int current = start;
  do {
    if (track[current].edge[DIR_AHEAD].dest != 0) {
      current = track[current].edge[DIR_AHEAD].dest->id;
    } else {
      current = -1;
    }
  } while(current >= 0 && track[current].type != NODE_SENSOR && track[current].type != NODE_BRANCH);

  return current;
}

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
  }
}


void sensor_saver_task() {
  // FIXME: priority
  logged_sensors = 0;
  last_logged_sensors = SENSOR_LOG_LENGTH - 1; // goes around
  int stopper_tid = Create(2, stopper);
  RegisterAs(SENSOR_SAVER);
  char req_buf[1024];
  packet_t *packet = (packet_t *) req_buf;
  int lastSensorTime = -1;
  int sender;

  int C14 = Name2Node("C14");
  int D12 = Name2Node("D12");
  int C8 = Name2Node("C8");
  int C15 = Name2Node("C15");
  int E8 = Name2Node("E8");
  int C10 = Name2Node("C10");
  int B1 = Name2Node("B1");
  int E14 = Name2Node("E14");

  int prevSensor[NUM_SENSORS][2];
  int sensorDistances[NUM_SENSORS][2];

  for (int i = 0; i < 80; i++) {
    prevSensor[i][0] = -1;
    prevSensor[i][1] = -1;
  }

  for (int i = 0; i < 80; i++) {
    int next1 = -1;
    int next2 = -1;

    int next = findSensorOrBranch(i);
    if (track[next].type == NODE_BRANCH) {
      next1 = track[next].edge[DIR_STRAIGHT].dest->id;
      next2 = track[next].edge[DIR_CURVED].dest->id;
      if (track[next1].type != NODE_SENSOR) {
        next1 = findSensorOrBranch(next1);
      }
      if (track[next2].type != NODE_SENSOR) {
        next2 = findSensorOrBranch(next2);
      }
      //KASSERT(next1 >= -1 && next1 < NUM_SENSORS, "next1 broken, got %d, started at %d, intermediary %d", next1, i, next);
      //KASSERT(next2 >= -1 && next2 < NUM_SENSORS, "next2 broken, got %d, started at %d, intermediary %d", next2, i, next);
    } else {
      next1 = next;
      //KASSERT(next1 >= -1 && next1 < NUM_SENSORS, "next1 broken, got %d", next1);
    }

    if (next1 >= 0 && next1 < NUM_SENSORS) {
      for (int j = 0; j < 2; j++) {
        if (prevSensor[next1][j] == -1) {
          prevSensor[next1][j] = i;
        }
      }
    }
    if (next2 >= 0 && next2 < NUM_SENSORS) {
      for (int j = 0; j < 2; j++) {
        if (prevSensor[next2][j] == -1) {
          prevSensor[next2][j] = i;
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

  GetPath(&p, Name2Node("E8"), Name2Node("C14"));
  int E8_C14_dist = p.dist;

  GetPath(&p, Name2Node("C15"), Name2Node("D12"));
  int C15_D12_dist = p.dist;

  GetPath(&p, Name2Node("E14"), Name2Node("E8"));
  int E14_E8_dist = p.dist;

  GetPath(&p, Name2Node("B1"), Name2Node("E14"));
  int B1_E14_dist = p.dist;

  GetPath(&p, Name2Node("E14"), Name2Node("E9"));
  int E14_E9_dist = p.dist;

  GetPath(&p, Name2Node("E14"), Name2Node("C14"));
  int E14_C14_dist = p.dist;

  GetPath(&p, Name2Node("C14"), Name2Node("C10"));
  int C14_C10_dist = p.dist;

  DECLARE_BASIS_NODE(basis_node);

  while (true) {
    ReceiveS(&sender, req_buf);
    switch (packet->type) {
    case SENSOR_DATA: {
          sensor_data_t * data = (sensor_data_t *) req_buf;
          int curr_time = Time();
          set_location(active_train, data->sensor_no);
          sensor_reading_timestamps[data->sensor_no] = curr_time;
          TriggerSensor(data->sensor_no, curr_time);
          if (data->sensor_no == basis_node && set_to_stop) {
            set_to_stop = false;
            GetPath(&p, basis_node, stop_on_node);
            SetPathSwitches(&p);
            int dist_to_dest = p.dist;
            int remaining_mm = dist_to_dest - StoppingDistance(active_train, active_speed);
            int velocity = Velocity(active_train, active_speed);
            // * 100 in order to get the amount of ticks (10ms) we need to wait
            int wait_ticks = remaining_mm * 100 / velocity;
            RecordLogf("waiting %6d ticks to reach %4s\n\r", wait_ticks, p.dest->name);
            Send(stopper_tid, &active_train, sizeof(int), NULL, 0);
            Send(stopper_tid, &wait_ticks, sizeof(int), NULL, 0);
          }

          if (set_to_stop_from && data->sensor_no == stop_on_node) {
            set_to_stop_from = false;
            SetTrainSpeed(active_train, 0);
          }

          int velocity = 0;
          if (lastSensor != -1) {
            for (int i = 0; i < 2; i++) {
              if (prevSensor[data->sensor_no][i] == lastSensor) {
                int time_diff = sensor_reading_timestamps[data->sensor_no] - sensor_reading_timestamps[lastSensor];
                velocity = (sensorDistances[data->sensor_no][i] * 100) / time_diff;
                RecordLogf("Readings for %2d ~> %2d : time_diff=%5d velocity=%3dmm/s (curve)\n\r", prevSensor[data->sensor_no][i], data->sensor_no, time_diff*10, velocity);
              }
            }
          }
          if (velocity > 0 && (curr_time - velocity_reading_delay_until) > 400) {
            record_velocity_sample(active_train, active_speed, velocity);
          }

          // int time = Time();
          // int diffTime = time - lastSensorTime;
          // if (lastSensor > 0) {
          //   registerSample(data->sensor_no, lastSensor, diffTime, time);
          // }
          lastSensor = data->sensor_no;
          lastSensorTime = curr_time;
        break;
    } default:
      KASSERT(false, "Received unknown request type.");
    }
    ReplyN(sender);
  }
}
