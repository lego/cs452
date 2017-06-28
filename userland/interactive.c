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

#define BASIS_NODE "A4"
#define BASIS_NODE_NAME Name2Node(BASIS_NODE)
#define DECLARE_BASIS_NODE(name) int name = BASIS_NODE_NAME

// used for displaying the path, updated on 100ms intervals
path_t *current_path;
bool is_pathing;
int pathing_start_time;
bool clear_path_display;

// used for pathing -- the train properties and whether to stop, and where to
int active_train;
int active_speed;
int velocity_reading_delay_until;
int most_recent_sensor;
bool set_to_stop;
bool set_to_stop_from;
bool stop_on_node;

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
  COMMAND_STOP_FROM,
  COMMAND_SET_LOCATION,
  COMMAND_PRINT_VELOCITY,
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

#define PATH_LOG_X 60
#define PATH_LOG_Y 2

int path_display_pos;

void DisplayPath(path_t *p, int train, int speed, int start_time, int curr_time) {
  Putstr(COM2, SAVE_CURSOR);
  int i;
  for (i = 0; i < path_display_pos; i++) {
    MoveTerminalCursor(PATH_LOG_X, PATH_LOG_Y + i);
    Putstr(COM2, CLEAR_LINE_AFTER);
  }
  path_display_pos = 0;

  int stop_dist = StoppingDistance(train, speed);
  int velo = Velocity(train, speed);

  // amount of mm travelled at this speed for the amount of time passed
  int travelled_dist = ((curr_time - start_time) * velo) / 100;

  int dist_to_dest = p->dist;
  int remaining_mm = dist_to_dest - stop_dist - travelled_dist;
  int calculated_time = remaining_mm * 10 / velo;

  MoveTerminalCursor(PATH_LOG_X, PATH_LOG_Y);
  Putstr(COM2, "Path from ");
  Putstr(COM2, p->src->name);
  Putstr(COM2, " ~> ");
  Putstr(COM2, p->dest->name);
  Putstr(COM2, ". Total distance=");
  Puti(COM2, p->dist);
  // only show ETA for navigation
  if (train == -2) {
    Putstr(COM2, ". eta=");
    Puti(COM2, (calculated_time / 100) % 10);
    Puti(COM2, (calculated_time / 10) % 10);
    Putstr(COM2, ".");
    Puti(COM2, calculated_time % 10);
    Putstr(COM2, "s");
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

    int remaining_mm_to_node = dist_sum - stop_dist - travelled_dist;
    int eta_to_node = remaining_mm_to_node * 10 / velo;

    // skip printing this node if we're already past it
    // and also only skip if navigating
    if (remaining_mm_to_node < 0 && train != -2) {
      continue;
    }

    path_display_pos++;
    MoveTerminalCursor(PATH_LOG_X, PATH_LOG_Y + path_display_pos);

    if (p->nodes[i-1]->type == NODE_BRANCH) {
      Putstr(COM2, "    switch=");
      Puti(COM2, p->nodes[i-1]->num);
      Putstr(COM2, " needs to be ");
      Putc(COM2, dir);
      path_display_pos++;
      MoveTerminalCursor(PATH_LOG_X, PATH_LOG_Y + path_display_pos);
    }

    Putstr(COM2, "  node=");
    Putstr(COM2, p->nodes[i]->name);


    // print distance to individual node and time to it
    Putstr(COM2, "  dist=");
    Puti(COM2, dist_sum);
    Putstr(COM2, "mm");
    // only show ETA for navigating
    if (train != -2) {
      Putstr(COM2, "  eta=");
      Puti(COM2, (eta_to_node / 100) % 10);
      Puti(COM2, (eta_to_node / 10) % 10);
      Putstr(COM2, ".");
      Puti(COM2, eta_to_node % 10);
      Putstr(COM2, "s");
    }
  }

  Putstr(COM2, RECOVER_CURSOR);
}

void UpdateDisplayPath(path_t *p, int train, int speed, int start_time, int curr_time) {
  Putstr(COM2, SAVE_CURSOR);
  int i;

  int stop_dist = StoppingDistance(train, speed);
  int velo = Velocity(train, speed);

  // amount of mm travelled at this speed for the amount of time passed
  int travelled_dist = ((curr_time - start_time) * velo) / 100;

  int dist_to_dest = p->dist;
  int remaining_mm = dist_to_dest - stop_dist - travelled_dist;
  int calculated_time = remaining_mm * 10 / velo;

  int offset = jstrlen("Path from ") + jstrlen(p->src->name) + jstrlen(" ~> ") + jstrlen(p->dest->name) + jstrlen(". Total distance=");
  MoveTerminalCursor(PATH_LOG_X + offset, PATH_LOG_Y);
  Puti(COM2, p->dist);
  // only show ETA for navigation
  if (train == -2) {
    Putstr(COM2, ". eta=");
    Puti(COM2, (calculated_time / 100) % 10);
    Puti(COM2, (calculated_time / 10) % 10);
    Putstr(COM2, ".");
    Puti(COM2, calculated_time % 10);
    Putstr(COM2, "s" CLEAR_LINE_AFTER);
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

    int remaining_mm_to_node = dist_sum - stop_dist - travelled_dist;
    int eta_to_node = remaining_mm_to_node * 10 / velo;

    // skip printing this node if we're already past it
    // and also only skip if navigating
    // if (remaining_mm_to_node < 0 && train != -2) {
    //   continue;
    // }

    path_display_pos++;

    if (p->nodes[i-1]->type == NODE_BRANCH) {
      path_display_pos++;
    }

    int node_offset = jstrlen(" node=") + jstrlen(p->nodes[i]->name) + jstrlen("  dist=") + jstrlen(p->dest->name) + jstrlen(". Total distance=");

    MoveTerminalCursor(PATH_LOG_X + node_offset, PATH_LOG_Y + path_display_pos);

    Puti(COM2, dist_sum);
    Putstr(COM2, "mm");
    // only show ETA for navigating
    if (train != -2) {
      Putstr(COM2, "  eta=");
      Puti(COM2, (eta_to_node / 100) % 10);
      Puti(COM2, (eta_to_node / 10) % 10);
      Putstr(COM2, ".");
      Puti(COM2, eta_to_node % 10);
      Putstr(COM2, "s" CLEAR_LINE_AFTER);
    }
  }

  Putstr(COM2, RECOVER_CURSOR);
}

void ClearLastCmdMessage() {
  Putstr(COM2, SAVE_CURSOR);
  MoveTerminalCursor(0, COMMAND_LOCATION + 1);
  Putstr(COM2, RECOVER_CURSOR);
}

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
    // navigates the train to B
    return COMMAND_NAVIGATE;
  } else if (jstrcmp(command, "path")) {
    // runs the pathing algorithm from A to B
    return COMMAND_PATH;
  } else if (jstrcmp(command, "velo")) {
    // manually sets the velocity for a train speed
    return COMMAND_SET_VELOCITY;
  } else if (jstrcmp(command, "pvelo")) {
    // manually sets the velocity for a train speed
    return COMMAND_PRINT_VELOCITY;
  } else if (jstrcmp(command, "loc")) {
    // manually sets the location for a train
    return COMMAND_SET_LOCATION;
  } else if (jstrcmp(command, "stopdist")) {
    // manually sets the stopdistance for a train speed
    return COMMAND_SET_STOPPING_DISTANCE;
  } else if (jstrcmp(command, "stopfrom")) {
    // moves the train to a node and sends the stop command on arrival
    // (only works for sensor nodes)
    return COMMAND_STOP_FROM;
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

int sample_points[1000];
int samples = 0;

#define NUM_SENSORS 80

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

void sensor_saver() {
  RegisterAs(SENSOR_SAVER);
  interactive_req_t req;
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
    ReceiveS(&sender, req);
    switch (req.type) {
    case INT_REQ_SENSOR_UPDATE: {
          int curr_time = Time();
          set_location(active_train, req.argc);
          sensor_reading_timestamps[req.argc] = curr_time;
          if (req.argc == basis_node && set_to_stop) {
            set_to_stop = false;
            GetPath(&p, basis_node, stop_on_node);
            SetPathSwitches(&p);
            int dist_to_dest = p.dist;
            int remaining_mm = dist_to_dest - StoppingDistance(active_train, active_speed);
            int velocity = Velocity(active_train, active_speed);
            // * 100 in order to get the amount of ticks (10ms) we need to wait
            int wait_ticks = remaining_mm * 100 / velocity;
            RecordLog("waiting ");
            RecordLogi(wait_ticks);
            RecordLog(" ticks to reach ");
            RecordLog(p.dest->name);
            RecordLog("\n\r");
            Delay(wait_ticks);
            velocity_reading_delay_until = Time();
            SetTrainSpeed(active_train, 0);
            is_pathing = false;
            clear_path_display = true;
          }

          if (set_to_stop_from && req.argc == stop_on_node) {
            set_to_stop_from = false;
            SetTrainSpeed(active_train, 0);
          }

          int velocity = 0;
          if (lastSensor != -1) {
            for (int i = 0; i < 2; i++) {
              if (prevSensor[req.argc][i] == lastSensor) {
                int time_diff = sensor_reading_timestamps[req.argc] - sensor_reading_timestamps[lastSensor];
                velocity = (sensorDistances[req.argc][i] * 100) / time_diff;
                RecordLog("Readings for ");
                RecordLogi(prevSensor[req.argc][i]);
                RecordLog(" ~> ");
                RecordLogi(req.argc);
                RecordLog(" : time_diff=");
                RecordLogi(time_diff*10);
                RecordLog(" velocity=");
                RecordLogi(velocity);
                RecordLog("mm/s (curve)\n\r");
              }
            }
          }
          if (velocity > 0 && (curr_time - velocity_reading_delay_until) > 400) {
            record_velocity_sample(active_train, active_speed, velocity);
          }

          // int time = Time();
          // int diffTime = time - lastSensorTime;
          // if (lastSensor > 0) {
          //   registerSample(req.argc, lastSensor, diffTime, time);
          // }
          lastSensor = req.argc;
          lastSensorTime = curr_time;
        break;
    } default:
      KASSERT(false, "Received unknown request type.");
    }
    ReplyN(sender);
  }
}

void interactive() {
  path_t p;
  current_path = &p;
  active_train = 0;
  active_speed = 0;
  velocity_reading_delay_until = 0;
  // this needs to come before SENSOR SAVER due to Name2Node, so this should just be the first to happen
  InitNavigation();
  path_display_pos = 0;
  samples = 0;
  set_to_stop = false;
  set_to_stop_from = false;
  is_pathing = false;
  clear_path_display = false;
  int path_update_counter = 0;

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

  ClearLastCmdMessage();

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
        Putstr(COM2, CLEAR_LINE_BEFORE);
        MoveTerminalCursor(0, COMMAND_LOCATION + 1);
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
                Putstr(COM2, "Invalid train provided for tr: got ");
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
              RecordLog(". Resetting samples.\n\r");

              lastTrain = train;
              samples = 0;
              SetTrainSpeed(train, speed);
              active_train = train;
              active_speed = speed;
              velocity_reading_delay_until = Time();
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
                Putstr(COM2, "Invalid train provided: got ");
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

              if (train != active_train || active_speed <= 0 || WhereAmI(train) == -1) {
                Putstr(COM2, "Train must already be in motion and hit a sensor to path.");
                break;
              }

              RecordLog("Basis is");
              RecordLogi(BASIS_NODE_NAME);
              RecordLog(".\n\r");

              active_train = train;
              active_speed = speed;
              velocity_reading_delay_until = Time();
              SetTrainSpeed(train, speed);

              // get the path to BASIS_NODE, our destination point
              GetPath(&p, WhereAmI(train), BASIS_NODE_NAME);
              // set all the switches to go there
              SetPathSwitches(&p);
              // get the full path including BASIS_NODE and display it
              GetMultiDestinationPath(&p, WhereAmI(train), BASIS_NODE_NAME, dest_node_id);
              DisplayPath(&p, active_train, active_speed, 0, 0);
              is_pathing = true;
              pathing_start_time = Time();
              // set the trains destination, this makes the pathing logic fire
              // up when the train hits BASIS_NODE
              stop_on_node = dest_node_id;
              set_to_stop = true;
            }
            break;
          case COMMAND_STOP_FROM:
            {
              int status;
              int train = jatoui(req.arg1, &status);
              if (status != 0) {
                Putstr(COM2, "Invalid train provided: got ");
                Putstr(COM2, req.arg1);
                break;
              }

              if (train < 0 || train > 80) {
                Putstr(COM2, "Invalid train provided: got ");
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

              if (train != active_train || active_speed <= 0 || WhereAmI(train) != -1) {
                Putstr(COM2, "Train must already be in motion and hit a sensor to path.");
                break;
              }

              active_train = train;
              active_speed = speed;
              // get the path to the stopping from node
              GetPath(&p, WhereAmI(train), dest_node_id);
              // set the switches for that route
              SetPathSwitches(&p);
              // display the path
              DisplayPath(&p, active_train, active_speed, 0, 0);
              is_pathing = true;
              pathing_start_time = Time();
              stop_on_node = dest_node_id;
              set_to_stop_from = true;
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

              GetPath(&p, src_node_id, dest_node_id);
              DisplayPath(&p, -2, -2, 0, 0);
              is_pathing = false;
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

              Putstr(COM2, "Set velocity train=");
              Putstr(COM2, req.arg1);
              Putstr(COM2, " speed=");
              Putstr(COM2, req.arg2);
              Putstr(COM2, " to ");
              Putstr(COM2, req.arg3);
              Putstr(COM2, "mm/s ");

              set_velocity(train, speed, velocity);
              }
              break;
          case COMMAND_PRINT_VELOCITY:
              {
                int velocity = Velocity(active_train, active_speed);
                char buf[10];
                ji2a(velocity, buf);
                Putstr(COM2, "Velocity = ");
                Putstr(COM2, buf);
                Putstr(COM2, "mm/s");
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

                Putstr(COM2, "Set stopping distance train=");
                Putstr(COM2, req.arg1);
                Putstr(COM2, " speed=");
                Putstr(COM2, req.arg2);
                Putstr(COM2, " to ");
                Putstr(COM2, req.arg3);
                Putstr(COM2, "mm ");
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
        int cur_time = Time();
        DrawTime(cur_time);
        DrawIdlePercent();
        MoveTerminalCursor(20, COMMAND_LOCATION + 4);
        Putstr(COM2, CLEAR_LINE);
        char buf[12];
        ji2a((1000*predictionAccuracy), buf);
        Putstr(COM2, buf);
        MoveTerminalCursor(30, COMMAND_LOCATION + 4);
        ji2a((1000*offset), buf);
        Putstr(COM2, buf);

        int sum = 0; int i;
        for (i = 0; i < samples; i++) sum += sample_points[i];
        sum /= samples;
        MoveTerminalCursor(0, COMMAND_LOCATION + 8);
        Putstr(COM2, "Velocity sample avg. ");
        Puti(COM2, sum);
        Putstr(COM2, "mm/s for ");
        Puti(COM2, samples);
        Putstr(COM2, " samples." CLEAR_LINE_AFTER);
        Putstr(COM2, RECOVER_CURSOR);
        // only print every 2 * 100ms, too much printing otherwise
        if (is_pathing && path_update_counter >= 2) {
          path_update_counter = 0;
          UpdateDisplayPath(current_path, active_train, active_speed, pathing_start_time, cur_time);
        } else {
          path_update_counter++;
        }
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
