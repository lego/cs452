#include <basic.h>
#include <util.h>
#include <kernel.h>
#include <jstring.h>
#include <terminal.h>
#include <servers/uart_rx_server.h>
#include <servers/uart_tx_server.h>
#include <servers/nameserver.h>
#include <interactive/command_parser.h>
#include <interactive/commands.h>
#include <interactive/interactive.h>

#define MAX_COMMAND_LIMIT 30
#define BUFFER_LIMIT 512

// use for command parsing
// in particular this is where the command is string split
// and a reference to the arguments locations are passed around
char global_command_buffer[BUFFER_LIMIT];

#define DEF_COMMAND(cmd, val) if (jstrcmp(command, cmd)) { return val; } else

command_t get_command_type(char *command) {
  DEF_COMMAND("q", COMMAND_QUIT)
  DEF_COMMAND("tr", COMMAND_TRAIN_SPEED)
  DEF_COMMAND("cal", COMMAND_TRAIN_CALIBRATE)
  DEF_COMMAND("rv", COMMAND_TRAIN_REVERSE)
  DEF_COMMAND("sen", COMMAND_MANUAL_SENSE)
  DEF_COMMAND("sw", COMMAND_SWITCH_TOGGLE)
  DEF_COMMAND("swa", COMMAND_SWITCH_TOGGLE_ALL)
  DEF_COMMAND("clss", COMMAND_CLEAR_SENSOR_SAMPLES)
  DEF_COMMAND("clo", COMMAND_CLEAR_SENSOR_OFFSET)
  DEF_COMMAND("pss", COMMAND_PRINT_SENSOR_SAMPLES)
  DEF_COMMAND("psm", COMMAND_PRINT_SENSOR_MULTIPLIERS)
  // navigates the train to B
  DEF_COMMAND("nav", COMMAND_NAVIGATE)
  // runs the pathing algorithm from A to B
  DEF_COMMAND("path", COMMAND_PATH)
  // manually sets the velocity for a train speed
  DEF_COMMAND("velo", COMMAND_SET_VELOCITY)
  // manually sets the velocity for a train speed
  DEF_COMMAND("pvelo", COMMAND_PRINT_VELOCITY)
  // manually sets the location for a train
  DEF_COMMAND("loc", COMMAND_SET_LOCATION)
  // manually sets the stopdistance for a train speed
  DEF_COMMAND("stopdistoff", COMMAND_STOPPING_DISTANCE_OFFSET)
  // manually sets the stopdistance for a train speed
  DEF_COMMAND("stopdist", COMMAND_SET_STOPPING_DISTANCE)
  // manually sets the stopdistance for a train speed
  DEF_COMMAND("stopdistn", COMMAND_SET_STOPPING_DISTANCEN)
  // moves the train to a node and sends the stop command on arrival
  // (only works for sensor nodes)
  DEF_COMMAND("stopfrom", COMMAND_STOP_FROM) {
    // KASSERT(false, "Command not valid: %s", command);
    return COMMAND_INVALID;
  }
}


parsed_command_t figure_out_command(char *command_buffer) {
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
  jstrsplit_buf(command_buffer, ' ', global_command_buffer, BUFFER_LIMIT);

  int argc = 0;
  while (global_command_buffer[argc] != '\0') argc++;
  // we don't count the first argument (the command name) in argc
  argc--;

  char *first_command = global_command_buffer + global_command_buffer[0];

  parsed_command_t cmd;
  cmd.packet.type = PARSED_COMMAND;
  cmd.type = get_command_type(first_command);
  cmd.argc = argc;
  cmd.cmd = global_command_buffer + global_command_buffer[0];

  for (int i = 0; i < MAX_ARGV; i++) {
    if (argc > i)
      cmd.argv[i] = global_command_buffer + global_command_buffer[i+1];
    else
      cmd.argv[i] = NULL;
  }

  return cmd;
}

void echo_character(int interactive_tid, char c) {
  interactive_echo_t echo_data;
  echo_data.packet.type = INTERACTIVE_ECHO;
  echo_data.echo[0] = c;
  echo_data.echo[1] = '\0';
  SendSN(interactive_tid, echo_data);
}

void echo_backspace(int interactive_tid) {
  interactive_echo_t echo_data;
  echo_data.packet.type = INTERACTIVE_ECHO;
  echo_data.echo[0] = BACKSPACE_CH;
  echo_data.echo[1] = ' ';
  echo_data.echo[2] = BACKSPACE_CH;
  echo_data.echo[3] = '\0';
  SendSN(interactive_tid, echo_data);
}

void command_parser_task() {
  int tid = MyTid();
  int command_interpreter_tid = WhoIsEnsured(NS_COMMAND_INTERPRETER);
  int interactive_tid = WhoIsEnsured(NS_INTERACTIVE);

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

        echo_backspace(interactive_tid);
        continue;
      }

      // if not alphanumeric or space, ignore
      if (!is_alphanumeric(c) && c != ' ' && c != '-') {
        continue;
      }

      echo_character(interactive_tid, c);

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

    parsed_command_t command_struct = figure_out_command(command_buffer);
    Send(command_interpreter_tid, &command_struct, sizeof(command_struct), NULL, 0);
    input_size = 0;
  }
}
