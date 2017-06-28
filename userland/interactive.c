#include <io.h>
#include <basic.h>
#include <bwio.h>
#include <interactive.h>
#include <servers/nameserver.h>
#include <servers/clock_server.h>
#include <terminal.h>
#include <servers/uart_rx_server.h>
#include <servers/uart_tx_server.h>
#include <train_controller.h>
#include <kernel.h>
#include <trains/navigation.h>
#include <jstring.h>
#include <priorities.h>

#define NUM_SWITCHES 22

// use for command parsing
// in particular this is where the command is string split
// and a reference to the arguments locations are passed around
char global_command_buffer[512];

enum interactive_req_type_t {
  INT_REQ_SENSOR_UPDATE,
  INT_REQ_COMMAND,
  INT_REQ_ECHO,
  INT_REQ_TIME,
}; typedef int interactive_req_type_t;

enum command_t {
  COMMAND_INVALID,
  COMMAND_QUIT,
  COMMAND_TRAIN_SPEED,
  COMMAND_TRAIN_REVERSE,
  COMMAND_SWITCH_TOGGLE,
  COMMAND_SWITCH_TOGGLE_ALL,
  COMMAND_CLEAR_SENSOR_SAMPLES,
  COMMAND_CLEAR_SENSOR_OFFSET,
  COMMAND_PRINT_SENSOR_SAMPLES,
  COMMAND_PRINT_SENSOR_MULTIPLIERS,
  COMMAND_HELP,
  COMMAND_SET_LOCATION,
  COMMAND_SET_VELOCITY,
  COMMAND_SET_STOPPING_DISTANCE,
  COMMAND_UPTIME,
  COMMAND_STATUS,
  COMMAND_NAVIGATE,
  COMMAND_PATH,
}; typedef int command_t;

typedef struct {
  interactive_req_type_t type;

  // FIXME: figure out a better way to not put all params in one struct
  // but use different structs

  // INT_REQ_COMMAND data
  command_t command_type;
  int argc;
  char *arg1;
  char *arg2;
  char *arg3;

  // INT_REQ_ECHO data
  char echo[4];
} interactive_req_t;

command_t get_command_type(char *command) {
  if (jstrcmp(command, "q")) {
    return COMMAND_QUIT;
  } else if (jstrcmp(command, "tr")) {
    return COMMAND_TRAIN_SPEED;
  } else if (jstrcmp(command, "rv")) {
    return COMMAND_TRAIN_REVERSE;
  } else if (jstrcmp(command, "sw")) {
    return COMMAND_SWITCH_TOGGLE;
  } else if (jstrcmp(command, "swa")) {
    return COMMAND_SWITCH_TOGGLE_ALL;
  } else if (jstrcmp(command, "clss")) {
    return COMMAND_CLEAR_SENSOR_SAMPLES;
  } else if (jstrcmp(command, "clo")) {
    return COMMAND_CLEAR_SENSOR_OFFSET;
  } else if (jstrcmp(command, "pss")) {
    return COMMAND_PRINT_SENSOR_SAMPLES;
  } else if (jstrcmp(command, "psm")) {
    return COMMAND_PRINT_SENSOR_MULTIPLIERS;
  } else if (jstrcmp(command, "nav")) {
    return COMMAND_NAVIGATE;
  } else if (jstrcmp(command, "path")) {
    return COMMAND_PATH;
  } else if (jstrcmp(command, "velo")) {
    return COMMAND_SET_VELOCITY;
  } else if (jstrcmp(command, "loc")) {
    return COMMAND_SET_LOCATION;
  } else if (jstrcmp(command, "stop")) {
    return COMMAND_SET_STOPPING_DISTANCE;
  } else {
    // KASSERT(false, "Command not valid: %s", command);
    return COMMAND_HELP;
  }
}

interactive_req_t figure_out_command(char *command_buffer) {
  // this does some magic to do a string split without malloc
  // in particular the prefix of the string is an array of bytes that are the
  // offsets of each string part, this array ends in '\0'. Then after are the
  // null terminated strings.
  //
  // e.g.
  //  5 12 17 '\0' H E L L O '\0' B Y E '\0' H I '\0'
  // byte 5, str 0 ^      byte 12 ^  byte 17 ^
  //
  // in order to access the strings
  // (buf + buf[0]) => "Hello"
  // (buf + buf[1]) => "Bye"
  // (buf + buf[2]) => "Hi"
  jstrsplit_buf(command_buffer, ' ', global_command_buffer, 512);

  int argc = 0;
  while (global_command_buffer[argc] != '\0') argc++;
  // we don't count the first argument (the command name) in argc
  argc--;

  char *first_command = global_command_buffer + global_command_buffer[0];

  interactive_req_t cmd;
  cmd.type = INT_REQ_COMMAND;
  cmd.command_type = get_command_type(first_command);
  cmd.argc = argc;
  // FIXME: this is very hardcoded and could be fixed.
  if (argc >= 1) {
    cmd.arg1 = global_command_buffer + global_command_buffer[1];
  } else {
    cmd.arg1 = NULL;
  }
  if (argc >= 2) {
    cmd.arg2 = global_command_buffer + global_command_buffer[2];
  } else {
    cmd.arg2 = NULL;
  }
  if (argc >= 3) {
    cmd.arg3 = global_command_buffer + global_command_buffer[3];
  } else {
    cmd.arg3 = NULL;
  }
  return cmd;
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

#define MAX_COMMAND_LIMIT 20

void command_parser() {
  int tid = MyTid();
  interactive_req_t echo_cmd;
  echo_cmd.type = INT_REQ_ECHO;
  int parent = MyParentTid();
  char command_buffer[MAX_COMMAND_LIMIT];
  log_task("command parser initialized", tid);
  // for every command
  while (true) {
    int input_size = 0;
    bool command_done = false;
    // for every character input
    while (!command_done) {
      char c = Getc(COM2);

      if (c == CARRIAGE_RETURN_CH || c == NEWLINE_CH) {
        // if sent EOL, finish command, don't put \r onto string
        command_done = true;
        continue;
      }

      // if backspace or delete, echo back to delete the character
      if (c == BACKSPACE_CH || c == DELETE_CH) {
        // NOTE: check before echo-ing otherwise we may echo a backspace when the size is empty
        // so do nothing
        if (input_size <= 0) {
          continue;
        }
        input_size--;

        // echo back a backspace
        echo_cmd.echo[0] = BACKSPACE_CH;
        echo_cmd.echo[1] = ' ';
        echo_cmd.echo[2] = BACKSPACE_CH;
        echo_cmd.echo[3] = '\0';
        Send(parent, &echo_cmd, sizeof(echo_cmd), NULL, 0);
        continue;
      }

      // if not alphanumeric or space, ignore
      if (!is_alphanumeric(c) && c != ' ') {
        continue;
      }

      // echo back character
      echo_cmd.echo[0] = c;
      echo_cmd.echo[1] = '\0';
      Send(parent, &echo_cmd, sizeof(echo_cmd), NULL, 0);

      command_buffer[input_size++] = c;
      if (input_size >= MAX_COMMAND_LIMIT) {
        command_done = true;
        continue;
      }
    }

    // PARSE COMMAND

    // if command is empty, just skip the command parsing
    if (input_size == 0) {
      continue;
    }

    // set end of string
    command_buffer[input_size] = '\0';

    interactive_req_t command_struct = figure_out_command(command_buffer);
    Send(parent, &command_struct, sizeof(command_struct), NULL, 0);
    input_size = 0;
  }
}

void sensor_reader() {
  int tid = MyTid();
  int parent = MyParentTid();
  int sensor_saver_tid = WhoIs(SENSOR_SAVER);
  interactive_req_t req;
  req.type = INT_REQ_SENSOR_UPDATE;
  log_task("sensor_reader initialized parent=%d", tid, parent);
  int oldSensors[5];
  int sensors[5];
  Delay(50); // Wait half a second for old COM1 input to be read
  while (true) {
    log_task("sensor_reader sleeping", tid);
    ClearRx(COM1);
    Putc(COM1, 0x85);
    int queueLength = 0;
    const int maxTries = 100;
    int i;
    for (i = 0; i < maxTries && queueLength < 10; i++) {
      Delay(1);
      queueLength = GetRxQueueLength(COM1);
    }
    // We tried to read but failed to get the correct number of bytes, ABORT
    if (i == maxTries) {
      continue;
    }
    log_task("sensor_reader reading", tid);
    for (int i = 0; i < 5; i++) {
      char high = Getc(COM1);
      char low = Getc(COM1);
      sensors[i] = (high << 8) | low;
    }
    log_task("sensor_reader read", tid);
    for (int i = 0; i < 5; i++) {
      if (sensors[i] != oldSensors[i]) {
        for (int j = 0; j < 16; j++) {
          if ((sensors[i] & (1 << j)) & ~(oldSensors[i] & (1 << j))) {
            req.argc = i*16+(15-j);
            Send(sensor_saver_tid, &req, sizeof(req), NULL, 0);
          }
        }
      }
      oldSensors[i] = sensors[i];
    }
  }
}

void time_keeper() {
  int tid = MyTid();
  int parent = MyParentTid();
  interactive_req_t req;
  req.type = INT_REQ_TIME;
  log_task("time_keeper initialized parent=%d", tid, parent);
  while (true) {
    log_task("time_keeper sleeping", tid);
    Delay(10);
    log_task("time_keeper sending", tid);
    Send(parent, &req, sizeof(req), NULL, 0);
  }
}

static void insert_two_char_int(int b, char *buf) {
  KASSERT(0 <= b && b < 100, "Provided number was outside of range: got %d", b);
  buf[0] = '0' + (b / 10);
  buf[1] = '0' + (b % 10);
}

void ticks2a(int ticks, char *buf, int buf_size) {
  KASSERT(buf_size >= 10, "Buffer provided is not big enough. Got %d, wanted >=10", buf_size);
  KASSERT(ticks < 360000, "Uptime exceeded an hour. Can't handle time display, ticks (%d) >= 360000", ticks);

  int minutes = (ticks / 6000) % 24;
  int seconds = (ticks / 100) % 60;


  insert_two_char_int(minutes, buf);
  // Insert semicolon
  buf[2] = ':';
  insert_two_char_int(seconds, buf + 3);
  // Insert space
  buf[5] = ' ';
  insert_two_char_int(ticks % 100, buf + 6);
  buf[8] = '0';
  buf[9] = '\0';
}

void DrawInitialScreen() {
  Putstr(COM2, SAVE_TERMINAL);
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
  char buf[12];
  ticks2a(t, buf, 12);
  Putstr(COM2, buf);
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
  Putc(COM2, '0' + (idle_percent / 100) % 10);
  Putc(COM2, '0' + (idle_percent / 10) % 10);
  Putc(COM2, '.');
  Putc(COM2, '0' + idle_percent % 10);
  Putc(COM2, '%');
}

void SetSwitchAndRender(int sw, int state) {
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
  SetSwitch(sw, state);
}

void initSwitches(int *initSwitches) {
  for (int i = 0; i < NUM_SWITCHES; i++) {
    int switchNumber = i+1;
    if (switchNumber >= 19) {
      switchNumber += 134; // 19 -> 153, etc
    }
    SetSwitchAndRender(switchNumber, initSwitches[i]);
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

void sensor_saver() {
  RegisterAs(SENSOR_SAVER);
  interactive_req_t req;
  int lastSensorTime = -1;
  int sender;

  int C14 = Name2Node("C14");
  int D12 = Name2Node("D12");
  int C8 = Name2Node("C8");
  int C15 = Name2Node("C15");
  int E11 = Name2Node("E11");
  int E8 = Name2Node("E8");
  int C10 = Name2Node("C10");
  int B1 = Name2Node("B1");
  int E14 = Name2Node("E14");

  path_t p;
  GetPath(&p, E8, C14);
  int C14_dist = p.dist;

  GetPath(&p, C15, D12);
  int D12_dist = p.dist;

  GetPath(&p, E11, E8);
  int E8_dist = p.dist;

  GetPath(&p, B1, E14);
  int E14_dist = p.dist;

  while (true) {
    ReceiveS(&sender, req);
    switch (req.type) {
    case INT_REQ_SENSOR_UPDATE: {

          // if (req.argc == 55) { // D7
          //   SetTrainSpeed(58, 1);
          //   // Delay(1);
          //   // SetTrainSpeed(58, 0);
          // }

          int curr_time = Time();
          sensor_reading_timestamps[req.argc] = curr_time;
          // if (req.argc == C10) {
          //   int remaining_mm = 280;
          //   int velocity = 480;
          //   int wait_ticks = remaining_mm * 100 / velocity;
          //   Delay(wait_ticks);
          //   SetTrainSpeed(69, 0);
          // }

          if (req.argc == C14) {
            int time_diff = sensor_reading_timestamps[C14] - sensor_reading_timestamps[E8];
            int velocity = (C14_dist * 100) / time_diff;

            RecordLog("Readings for E8 ~> C14: time_diff=");
            RecordLogi(time_diff*10);
            RecordLog(" velocity=");
            RecordLogi(velocity);
            RecordLog("mm/s\n\r");
          } else if (req.argc == D12) {
            int time_diff = sensor_reading_timestamps[D12] - sensor_reading_timestamps[C15];
            int velocity = (D12_dist * 100) / time_diff;

            RecordLog("Readings for C15 ~> D12: time_diff=");
            RecordLogi(time_diff*10);
            RecordLog(" velocity=");
            RecordLogi(velocity);
            RecordLog("mm/s\n\r");
          } else if (req.argc == E8) {
            int time_diff = sensor_reading_timestamps[E8] - sensor_reading_timestamps[E11];
            int velocity = (E8_dist * 100) / time_diff;

            RecordLog("Readings for E11 ~> E8 : time_diff=");
            RecordLogi(time_diff*10);
            RecordLog(" velocity=");
            RecordLogi(velocity);
            RecordLog("mm/s (curve)\n\r");
          } else if (req.argc == E14) {
            int time_diff = sensor_reading_timestamps[E14] - sensor_reading_timestamps[B1];
            int velocity = (E14_dist * 100) / time_diff;

            RecordLog("Readings for B1 ~> E14 : time_diff=");
            RecordLogi(time_diff*10);
            RecordLog(" velocity=");
            RecordLogi(velocity);
            RecordLog("mm/s (curve)\n\r");
          }

          // int time = Time();
          // int diffTime = time - lastSensorTime;
          // if (lastSensor > 0) {
          //   registerSample(req.argc, lastSensor, diffTime, time);
          // }
          // lastSensor = req.argc;
          // lastSensorTime = time;
        break;
    } default:
      KASSERT(false, "Received unknown request type.");
    }
    ReplyN(sender);
  }
}

void interactive() {
  // this needs to come before SENSOR SAVER due to Name2Node, so this should just be the first to happen
  InitNavigation();


  int tid = MyTid();
  int command_parser_tid = Create(7, command_parser);
  int time_keeper_tid = Create(7, time_keeper);
  int sensor_saver_tid = Create(PRIORITY_UART1_RX_SERVER, sensor_saver);
  int sensor_reader_tid = Create(PRIORITY_UART1_RX_SERVER+1, sensor_reader);
  int sender;
  idle_execution_time = 0;
  last_time_idle_displayed = 0;

  log_task("interactive initialized time_keeper=%d", tid, time_keeper_tid);
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
  initialSwitchStates[12] = SWITCH_CURVED;
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


  interactive_req_t req;

  int lastSensor = -1;
  int lastSensorTime = -1;
  clearBuckets();

  while (true) {
    ReceiveS(&sender, req);
    log_task("got msg type=%d", tid, req.type);
    switch (req.type) {
      case INT_REQ_ECHO:
        // log_task("interactive is echoing", tid);
        Putstr(COM2, req.echo);
        break;
      case INT_REQ_COMMAND:
        // TODO: this switch statement of commands should be yanked out, it's very long and messy
        MoveTerminalCursor(0, COMMAND_LOCATION + 1);
        Putstr(COM2, CLEAR_LINE);
        switch (req.command_type) {
          case COMMAND_QUIT:
            Putstr(COM2, "\033[2J\033[HRunning quit");
            Delay(5);
            ExitKernel();
            break;
          case COMMAND_TRAIN_SPEED:
            {
              int status;
              int train = jatoui(req.arg1, &status);
              if (status != 0) {
                Putstr(COM2, "Invalid train provided for tr: got ");
                Putstr(COM2, req.arg1);
                break;
              }
              int speed = jatoui(req.arg2, &status);
              if (status != 0) {
                Putstr(COM2, "Invalid speed provided for tr: got ");
                Putstr(COM2, req.arg1);
                break;
              }

              if (train < 0 || train > 80) {
                Putstr(COM2, "Invalid speed provided for tr: got ");
                Putstr(COM2, req.arg1);
                Putstr(COM2, " expected 1-80");
                break;
              }

              if (speed < 0 || speed > 14) {
                Putstr(COM2, "Invalid speed provided for tr: got ");
                Putstr(COM2, req.arg1);
                Putstr(COM2, " expected 0-14");
                break;
              }

              Putstr(COM2, "Set train ");
              Putstr(COM2, req.arg1);
              Putstr(COM2, " to speed ");
              Putstr(COM2, req.arg2);

              RecordLog("Set train ");
              RecordLog(req.arg1);
              RecordLog(" to speed ");
              RecordLog(req.arg2);
              RecordLog("\n\r");

              lastTrain = train;
              SetTrainSpeed(train, speed);
            }
            break;
          case COMMAND_TRAIN_REVERSE:
            {
              int status;
              int train = jatoui(req.arg1, &status);
              if (status != 0) {
                Putstr(COM2, "Invalid train provided for tr: got ");
                Putstr(COM2, req.arg1);
                break;
              }
              if (train < 0 || train > 80) {
                Putstr(COM2, "Invalid speed provided for tr: got ");
                Putstr(COM2, req.arg1);
                Putstr(COM2, " expected 1-80");
                break;
              }

              Putstr(COM2, "Train ");
              Putstr(COM2, req.arg1);
              Putstr(COM2, " reverse");
              ReverseTrain(train, 14);
            }
            break;
          case COMMAND_SWITCH_TOGGLE:
            {
              int sw = ja2i(req.arg1);
              if (jstrcmp(req.arg2, "c")) {
                SetSwitchAndRender(sw, SWITCH_CURVED);
              } else if (jstrcmp(req.arg2, "s")) {
                SetSwitchAndRender(sw, SWITCH_STRAIGHT);
              } else {
                Putstr(COM2, "Invalid switch option provided for sw: got '");
                Putstr(COM2, req.arg1);
                Putstr(COM2, "' expected 'c' or 's'");
                break;
              }

              Putstr(COM2, "Set switch ");
              Putstr(COM2, req.arg1);
              Putstr(COM2, " to ");
              Putstr(COM2, req.arg2);
            }
            break;
          case COMMAND_SWITCH_TOGGLE_ALL:
            {
              Putstr(COM2, "Set all switches to ");
              Putstr(COM2, req.arg1);
              int state = SWITCH_CURVED;
              if (jstrcmp(req.arg1, "c")) {
                state = SWITCH_CURVED;
              } else if (jstrcmp(req.arg1, "s")) {
                state = SWITCH_STRAIGHT;
              } else {
                Putstr(COM2, "Invalid switch option provided for sw: got '");
                Putstr(COM2, req.arg1);
                Putstr(COM2, "' expected 'c' or 's'");
                break;
              }
              for (int i = 0; i < NUM_SWITCHES; i++) {
                int switchNumber = i+1;
                if (switchNumber >= 19) {
                  switchNumber += 134; // 19 -> 153, etc
                }
                SetSwitchAndRender(switchNumber, state);
                Delay(6);
              }
            }
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
                ji2a((total/bucketSize[i]), buf);
                Putstr(COM2, buf);
                Putstr(COM2, "  -  ");
                ji2a((int)(avg*1000), buf);
                Putstr(COM2, buf);

                //for (int j = 0; j < bucketSize[i]; j++) {
                //  MoveTerminalCursor((j+1) * 6, COMMAND_LOCATION + 3 + i);
                //  char buf[10];
                //  ji2a(bucketSamples[i*SAMPLES+j], buf);
                //  Putstr(COM2, buf);
                //}
              }
              MoveTerminalCursor(0, COMMAND_LOCATION + 3);
              ji2a(speedTotal/n, buf);
              Putstr(COM2, buf);
              Putstr(COM2, RECOVER_CURSOR);
            }
            break;
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
                ji2a((int)(avg*1000), buf);
                Putstr(COM2, buf);
              }
              Putstr(COM2, RECOVER_CURSOR);
            }
            break;
          case COMMAND_NAVIGATE:
            {
              int status;
              int train = jatoui(req.arg1, &status);
              if (status != 0) {
                Putstr(COM2, "Invalid train provided: got ");
                Putstr(COM2, req.arg1);
                break;
              }

              if (train < 0 || train > 80) {
                Putstr(COM2, "Invalid speed provided: got ");
                Putstr(COM2, req.arg1);
                Putstr(COM2, " expected 1-80");
                break;
              }

              int speed = jatoui(req.arg2, &status);
              if (status != 0) {
                Putstr(COM2, "Invalid speed provided: got ");
                Putstr(COM2, req.arg2);
                break;
              }

              if (speed < 0 || speed > 14) {
                Putstr(COM2, "Invalid speed provided: got ");
                Putstr(COM2, req.arg2);
                Putstr(COM2, " expected 0-14");
                break;
              }
              int dest_node_id = Name2Node(req.arg3);
              if (dest_node_id == -1) {
                Putstr(COM2, "Invalid dest node: got ");
                Putstr(COM2, req.arg3);
                break;
              }

              path_t p;
              GetPath(&p, WhereAmI(train), dest_node_id);

              Putstr(COM2, "Navigating train ");
              Putstr(COM2, req.arg1);
              Putstr(COM2, " at speed ");
              Putstr(COM2, req.arg2);
              Putstr(COM2, " to ");
              Putstr(COM2, req.arg3);
              Putstr(COM2, ". dist=");
              char buf[10];
              ji2a(p.dist, buf);
              Putstr(COM2, buf);
              Putstr(COM2, " len=");
              ji2a(p.len, buf);
              Putstr(COM2, buf);
              Putstr(COM2, " path=");
              int k;
              for (k = 0; k < p.len; k++) {
                Putstr(COM2, p.nodes[k]->name);
                if (k < p.len - 1) {
                  Putstr(COM2, ",");
                }
              }
              Navigate(train, speed, WhereAmI(train), dest_node_id);
            }
            break;
          case COMMAND_PATH:
            {
              int src_node_id = Name2Node(req.arg1);
              if (src_node_id == -1) {
                Putstr(COM2, "Invalid src node: got ");
                Putstr(COM2, req.arg1);
                break;
              }

              int dest_node_id = Name2Node(req.arg2);
              if (dest_node_id == -1) {
                Putstr(COM2, "Invalid dest node: got ");
                Putstr(COM2, req.arg2);
                break;
              }

              path_t p;
              GetPath(&p, src_node_id, dest_node_id);

              Putstr(COM2, "Path ");
              Putstr(COM2, req.arg1);
              Putstr(COM2, " ~> ");
              Putstr(COM2, req.arg3);
              Putstr(COM2, ". dist=");
              char buf[10];
              ji2a(p.dist, buf);
              Putstr(COM2, buf);
              Putstr(COM2, " len=");
              ji2a(p.len, buf);
              Putstr(COM2, buf);
              Putstr(COM2, " route=");
              int k;
              for (k = 0; k < p.len; k++) {
                Putstr(COM2, p.nodes[k]->name);
                if (k < p.len - 1) {
                  Putstr(COM2, ",");
                }
              }
            }
            break;
          case COMMAND_SET_VELOCITY:
            {
              int status;
              int train = jatoui(req.arg1, &status);
              if (status != 0) {
                Putstr(COM2, "Invalid train provided: got ");
                Putstr(COM2, req.arg1);
                break;
              }

              if (train < 0 || train > 80) {
                Putstr(COM2, "Invalid speed provided: got ");
                Putstr(COM2, req.arg1);
                Putstr(COM2, " expected 1-80");
                break;
              }

              int speed = jatoui(req.arg2, &status);
              if (status != 0) {
                Putstr(COM2, "Invalid speed provided: got ");
                Putstr(COM2, req.arg2);
                break;
              }

              if (speed < 0 || speed > 14) {
                Putstr(COM2, "Invalid speed provided: got ");
                Putstr(COM2, req.arg2);
                Putstr(COM2, " expected 0-14");
                break;
              }

              int velocity = jatoui(req.arg3, &status);
              if (status != 0) {
                Putstr(COM2, "Invalid velocity provided: got ");
                Putstr(COM2, req.arg3);
                break;
              }

              Putstr(COM2, "Set velocity of ");
              Putstr(COM2, req.arg1);
              Putstr(COM2, " speed ");
              Putstr(COM2, req.arg2);
              Putstr(COM2, " to ");
              Putstr(COM2, req.arg3);
              Putstr(COM2, " mm/s ");

              set_velocity(train, speed, velocity);
              }
              break;
          case COMMAND_SET_LOCATION:
            {
              int status;
              int train = jatoui(req.arg1, &status);
              if (status != 0) {
                Putstr(COM2, "Invalid train provided: got ");
                Putstr(COM2, req.arg1);
                break;
              }

              if (train < 0 || train > 80) {
                Putstr(COM2, "Invalid speed provided: got ");
                Putstr(COM2, req.arg1);
                Putstr(COM2, " expected 1-80");
                break;
              }

              int node = Name2Node(req.arg2);
              if (node == -1) {
                Putstr(COM2, "Invalid location.");
                break;
              }

              set_location(train, node);
            }
            break;
          case COMMAND_SET_STOPPING_DISTANCE:
            {
                int status;
                int train = jatoui(req.arg1, &status);
                if (status != 0) {
                  Putstr(COM2, "Invalid train provided: got ");
                  Putstr(COM2, req.arg1);
                  break;
                }

                if (train < 0 || train > 80) {
                  Putstr(COM2, "Invalid speed provided: got ");
                  Putstr(COM2, req.arg1);
                  Putstr(COM2, " expected 1-80");
                  break;
                }

                int speed = jatoui(req.arg2, &status);
                if (status != 0) {
                  Putstr(COM2, "Invalid speed provided: got ");
                  Putstr(COM2, req.arg2);
                  break;
                }

                if (speed < 0 || speed > 14) {
                  Putstr(COM2, "Invalid speed provided: got ");
                  Putstr(COM2, req.arg2);
                  Putstr(COM2, " expected 0-14");
                  break;
                }

                int stopping_distance = jatoui(req.arg3, &status);
                if (status != 0) {
                  Putstr(COM2, "Invalid stopping distance provided: got ");
                  Putstr(COM2, req.arg3);
                  break;
                }

                Putstr(COM2, "Set stopping distance of ");
                Putstr(COM2, req.arg1);
                Putstr(COM2, " speed ");
                Putstr(COM2, req.arg2);
                Putstr(COM2, " to ");
                Putstr(COM2, req.arg3);
                Putstr(COM2, " mm ");
                set_stopping_distance(train, speed, stopping_distance);
              }
              break;
          default:
            Putstr(COM2, "Got invalid command=");
            Putstr(COM2, global_command_buffer + global_command_buffer[0]);
            break;
        }
        Putstr(COM2, "\n\r");
        Putstr(COM2, CLEAR_LINE);
        Putstr(COM2, "# ");
        break;
      case INT_REQ_TIME:
        // log_task("interactive is updating time", tid);
        Putstr(COM2, SAVE_CURSOR);
        DrawTime(Time());
        DrawIdlePercent();
        MoveTerminalCursor(20, COMMAND_LOCATION + 4);
        Putstr(COM2, CLEAR_LINE);
        char buf[12];
        ji2a((1000*predictionAccuracy), buf);
        Putstr(COM2, buf);
        MoveTerminalCursor(30, COMMAND_LOCATION + 4);
        ji2a((1000*offset), buf);
        Putstr(COM2, buf);
        Putstr(COM2, RECOVER_CURSOR);
        break;
      case INT_REQ_SENSOR_UPDATE:
        {
          Putstr(COM2, SAVE_CURSOR);
          MoveTerminalCursor(0, SENSOR_HISTORY_LOCATION + 1);
          Putstr(COM2, CLEAR_LINE);
          Putstr(COM2, "Sensors read! ");
          int time = Time();
          int diffTime = time - lastSensorTime;
          if (lastSensor > 0) {
            registerSample(req.argc, lastSensor, diffTime, time);
          }
          {
            int bucket = lastSensor/16;
            switch (bucket) {
              case 0:
                Putstr(COM2, "A");
                break;
              case 1:
                Putstr(COM2, "B");
                break;
              case 2:
                Putstr(COM2, "C");
                break;
              case 3:
                Putstr(COM2, "D");
                break;
              case 4:
                Putstr(COM2, "E");
                break;
            }
            char buf[10];
            ji2a((lastSensor%16)+1, buf);
            Putstr(COM2, " ");
            Putstr(COM2, buf);
          }
          Putstr(COM2, " -> ");
          {
            int bucket = req.argc/16;
            switch (bucket) {
              case 0:
                Putstr(COM2, "A");
                break;
              case 1:
                Putstr(COM2, "B");
                break;
              case 2:
                Putstr(COM2, "C");
                break;
              case 3:
                Putstr(COM2, "D");
                break;
              case 4:
                Putstr(COM2, "E");
                break;
            }
            char buf[10];
            ji2a((req.argc%16)+1, buf);
            Putstr(COM2, " ");
            Putstr(COM2, buf);
          }
          Putstr(COM2, " : ");
          char buf[10];
          ji2a(diffTime, buf);
          Putstr(COM2, buf);
          Putstr(COM2, RECOVER_CURSOR);
          lastSensor = req.argc;
          lastSensorTime = time;
        }
        break;
      default:
        KASSERT(false, "Bad type received: got type=%d", req.type);
        break;
    }
    // NOTE: this is after because if the command parser runs again
    // it can/will write over the global_command_buffer
    ReplyN(sender);

  }
}
