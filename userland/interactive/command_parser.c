#include <basic.h>
#include <util.h>
#include <kernel.h>
#include <jstring.h>
#include <terminal.h>
#include <servers/uart_rx_server.h>
#include <interactive/command_parser.h>
#include <interactive/interactive.h>

// use for command parsing
// in particular this is where the command is string split
// and a reference to the arguments locations are passed around
char global_command_buffer[512];

command_t get_command_type(char *command) {
  if (jstrcmp(command, "q")) {
    return COMMAND_QUIT;
  } else if (jstrcmp(command, "tr")) {
    return COMMAND_TRAIN_SPEED;
  } else if (jstrcmp(command, "rv")) {
    return COMMAND_TRAIN_REVERSE;
  } else if (jstrcmp(command, "sen")) {
    return COMMAND_MANUAL_SENSE;
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
  } else if (jstrcmp(command, "stopdistoff")) {
    // manually sets the stopdistance for a train speed
    return COMMAND_STOPPING_DISTANCE_OFFSET;
  } else if (jstrcmp(command, "stopdist")) {
    // manually sets the stopdistance for a train speed
    return COMMAND_SET_STOPPING_DISTANCE;
  } else if (jstrcmp(command, "stopdistn")) {
    // manually sets the stopdistance for a train speed
    return COMMAND_SET_STOPPING_DISTANCEN;
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

  cmd.cmd = global_command_buffer + global_command_buffer[0];

  return cmd;
}

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
      if (!is_alphanumeric(c) && c != ' ' && c != '-') {
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
