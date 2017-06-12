#include <basic.h>
#include <interactive.h>
#include <nameserver.h>
#include <clock_server.h>
#include <terminal.h>
#include <uart_rx_server.h>
#include <uart_tx_server.h>
#include <train_controller.h>
#include <kernel.h>
#include <jstring.h>

// Terminal locations
#define SWITCH_LOCATION 3
#define SENSOR_HISTORY_LOCATION 10
#define COMMAND_LOCATION 23

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
  COMMAND_HELP,
  COMMAND_UPTIME,
  COMMAND_STATUS,
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

#define MAX_COMMAND_LIMIT 80

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
      if (input_size == MAX_COMMAND_LIMIT) {
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
    const int maxTries = 5;
    int i;
    for (i = 0; i < maxTries && queueLength < 10; i++) {
      Delay(10);
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
            Send(parent, &req, sizeof(req), NULL, 0);
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

void move_cursor(unsigned int x, unsigned int y) {
  char bf[12];
  Putc(COM2, ESCAPE_CH);
  Putc(COM2, '[');

  ui2a(y, 10, bf);
  Putstr(COM2, bf);

  Putc(COM2, ';');

  ui2a(x, 10, bf);
  Putstr(COM2, bf);

  Putc(COM2, 'H');
}

void DrawInitialScreen() {
  Putstr(COM2, SAVE_TERMINAL);
  Putstr(COM2, "Time xx:xx xx0\n\r");
  Putstr(COM2, "Idle xx.x%\n\r");

  // Change BG colour
  Putstr(COM2, BLACK_FG);
  Putstr(COM2, WHITE_BG);
  move_cursor(0, SWITCH_LOCATION);
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
  move_cursor(0, SENSOR_HISTORY_LOCATION);
  Putstr(COM2, "Recent sensor triggers");

  move_cursor(0, COMMAND_LOCATION);
  Putstr(COM2, "Commands");
  // Reset BG colour
  Putstr(COM2, RESET_ATTRIBUTES);

  Putstr(COM2, "\n\r\n\r# ");

}

void DrawTime(int t) {
  // "Time 10:22 100"
  //       ^ 6th char
  move_cursor(6, 0);
  char buf[12];
  ticks2a(t, buf, 12);
  Putstr(COM2, buf);
}

// from main.c
#include <io.h>
extern volatile io_time_t idle_time_total;
extern volatile io_time_t time_since_idle_totalled;

void DrawIdlePercent() {
  // FIXME: is there a way to drop the use of kernel global variables here?

  // get idle total, current time, and total time for idle total start
  io_time_t idle_total = idle_time_total;
  io_time_t curr_time = io_get_time();
  io_time_t time_total = curr_time - time_since_idle_totalled;
  // reset counters
  time_since_idle_totalled = curr_time;
  idle_time_total = 0;

  int idle_percent = (idle_total * 1000) / time_total;

  // "Idle 99.9% "
  //       ^ 6th char
  move_cursor(0, 2);

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
    move_cursor(3+7*(index/6)+(index>=18?1:0), SWITCH_LOCATION+1+index%6);
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
    Delay(5);
  }
}

void interactive() {
  int tid = MyTid();
  int command_parser_tid = Create(7, &command_parser);
  int time_keeper_tid = Create(7, &time_keeper);
  int sensor_reader_tid = Create(7, &sensor_reader);
  int sender;

  log_task("interactive initialized time_keeper=%d", tid, time_keeper_tid);
  DrawInitialScreen();
  Putstr(COM2, SAVE_CURSOR);
  DrawTime(Time());
  Putstr(COM2, RECOVER_CURSOR);

  int initialSwitchStates[NUM_SWITCHES];
  initialSwitchStates[ 0] = SWITCH_CURVED;
  initialSwitchStates[ 1] = SWITCH_CURVED;
  initialSwitchStates[ 2] = SWITCH_CURVED;
  initialSwitchStates[ 3] = SWITCH_CURVED;
  initialSwitchStates[ 4] = SWITCH_CURVED;
  initialSwitchStates[ 5] = SWITCH_CURVED;
  initialSwitchStates[ 6] = SWITCH_CURVED;
  initialSwitchStates[ 7] = SWITCH_CURVED;
  initialSwitchStates[ 8] = SWITCH_CURVED;
  initialSwitchStates[ 9] = SWITCH_CURVED;
  initialSwitchStates[10] = SWITCH_CURVED;
  initialSwitchStates[11] = SWITCH_CURVED;
  initialSwitchStates[12] = SWITCH_CURVED;
  initialSwitchStates[13] = SWITCH_CURVED;
  initialSwitchStates[14] = SWITCH_CURVED;
  initialSwitchStates[15] = SWITCH_CURVED;
  initialSwitchStates[16] = SWITCH_CURVED;
  initialSwitchStates[17] = SWITCH_CURVED;
  initialSwitchStates[18] = SWITCH_STRAIGHT;
  initialSwitchStates[19] = SWITCH_CURVED;
  initialSwitchStates[20] = SWITCH_STRAIGHT;
  initialSwitchStates[21] = SWITCH_CURVED;
  initSwitches(initialSwitchStates);

  interactive_req_t req;

  while (true) {
    ReceiveS(&sender, req);
    log_task("got msg type=%d", tid, req.type);
    switch (req.type) {
      case INT_REQ_ECHO:
        // log_task("interactive is echoing", tid);
        Putstr(COM2, req.echo);
        break;
      case INT_REQ_COMMAND:
        move_cursor(0, COMMAND_LOCATION + 1);
        Putstr(COM2, CLEAR_LINE);
        switch (req.command_type) {
          case COMMAND_QUIT:
            Putstr(COM2, "\033[2J\033[HRunning quit");
            Delay(5);
            ExitKernel();
            break;
          case COMMAND_TRAIN_SPEED:
            {
              Putstr(COM2, "Set train ");
              Putstr(COM2, req.arg1);
              Putstr(COM2, " to speed ");
              Putstr(COM2, req.arg2);
              int train = ja2i(req.arg1);
              int speed = ja2i(req.arg2);
              SetTrainSpeed(train, speed);
            }
            break;
          case COMMAND_TRAIN_REVERSE:
            {
              Putstr(COM2, "Train ");
              Putstr(COM2, req.arg1);
              Putstr(COM2, " reverse");
              int train = ja2i(req.arg1);
              ReverseTrain(train, 14);
            }
            break;
          case COMMAND_SWITCH_TOGGLE:
            {
              Putstr(COM2, "Set switch ");
              Putstr(COM2, req.arg1);
              Putstr(COM2, " to ");
              Putstr(COM2, req.arg2);
              int sw = ja2i(req.arg1);
              if (jstrcmp(req.arg2, "c")) {
                SetSwitchAndRender(sw, SWITCH_CURVED);
              } else if (jstrcmp(req.arg2, "s")) {
                SetSwitchAndRender(sw, SWITCH_STRAIGHT);
              }
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
              }
              for (int i = 0; i < NUM_SWITCHES; i++) {
                int switchNumber = i+1;
                if (switchNumber >= 19) {
                  switchNumber += 134; // 19 -> 153, etc
                }
                SetSwitchAndRender(switchNumber, state);
                Delay(5);
              }
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
        move_cursor(0, COMMAND_LOCATION + 3);
        Putstr(COM2, CLEAR_LINE);
        char buf[10];
        ji2a(GetRxQueueLength(COM1), buf);
        Putstr(COM2, RECOVER_CURSOR);
        break;
      case INT_REQ_SENSOR_UPDATE:
        {
          Putstr(COM2, SAVE_CURSOR);
          move_cursor(0, SENSOR_HISTORY_LOCATION + 1);
          Putstr(COM2, CLEAR_LINE);
          Putstr(COM2, "Sensors read! ");
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
          Putstr(COM2, RECOVER_CURSOR);
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
