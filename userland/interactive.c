#include <basic.h>
#include <interactive.h>
#include <nameserver.h>
#include <clock_server.h>
#include <terminal.h>
#include <uart_rx_server.h>
#include <uart_tx_server.h>
#include <kernel.h>

// Terminal locations
#define SENSOR_HISTORY_LOCATION 10
#define COMMAND_LOCATION 23

enum interactive_req_type_t {
  INT_REQ_SENSOR_UPDATE,
  INT_REQ_COMMAND,
  INT_REQ_TIME,
}; typedef int interactive_req_type_t;

enum command_t {
  COMMAND_QUIT,
}; typedef int command_t;

typedef struct {
  interactive_req_type_t type;
} interactive_req_t;

typedef struct {
  interactive_req_type_t type;
  command_t command;
  // FIXME: add command arguments
} interactive_req_command_t;

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

// void command_parser() {
//   command = "";
//   while (true) {
//     c = GetC();
//     command.append(c);
//
//     if (end_of_command(command)) {
//       command_struct = figure_out_command(command);
//       Send(interactive, command_struct);
//       command = "";
//     }
//   }
// }

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
  KASSERT(buf_size >= 10, "Buffer provided is not big enough");
  KASSERT(ticks < 360000, "Uptime exceeded an hour. Can't handle time display.");

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
  Putstr(COM2, "Idle xx.x%%\n\r");

  // Change BG colour
  Putstr(COM2, BLACK_FG);
  Putstr(COM2, WHITE_BG);
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

io_time_t GetIdleTicks() {
  return idle_time_total;
}

void DrawIdlePercent() {
  io_time_t idle_total = GetIdleTicks();
  io_time_t curr_time = io_get_time();
  io_time_t time_total = curr_time - time_since_idle_totalled;
  time_since_idle_totalled = curr_time;

  int idle_percent = (time_total * 100) / idle_total;

  // "Idle 99.9% "
  //       ^ 6th char
  Putstr(COM2, "\n\rIdle ");
  char tmp[12];
  i2a(idle_percent, tmp);
  Putstr(COM2, tmp);

  // Putc(COM2, '0' + (idle_percent / 100) % 10);
  // Putc(COM2, '0' + (idle_percent / 10) % 10);
  // Putc(COM2, '.');
  // Putc(COM2, '0' + idle_percent % 10);
}

void interactive() {
  int tid = MyTid();
  // int sensor_query_tid = Create(5, &sensor_query);
  // int command_parser_tid = Create(5, &command_parser);
  int time_keeper_tid = Create(5, &time_keeper);
  int sender;

  log_task("interactive initialized time_keeper=%d", tid, time_keeper_tid);
  DrawInitialScreen();
  DrawTime(Time());

  interactive_req_t req;

  while (true) {
    ReceiveS(&sender, req);
    log_task("got msg type=%d", tid, req.type);
    switch (req.type) {
      // case Command:
      //   result = ExecuteCommand(command);
      //   DrawCommandResult(result);
      //   break;
      // case Sensor:
      //   DrawSensorUpdate(update);
      //   break;
      case INT_REQ_TIME:
        log_task("interactive is updating time", tid);
        ReplyN(sender);
        Putstr(COM2, SAVE_CURSOR);
        DrawTime(Time());
        DrawIdlePercent();
        Putstr(COM2, RECOVER_CURSOR);
        break;
      default:
        KASSERT(false, "Bad type received: got type=%d", req.type);
        break;
    }
  }
}
