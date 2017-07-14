#pragma once

#include <packet.h>
#include <interactive/commands.h>

#define MAX_ARGV 4

typedef struct {
  // PARSED_COMMAND
  packet_t packet;

  command_t type;
  char *cmd;
  int argc;
  char *argv[MAX_ARGV];
} parsed_command_t;

command_t get_command_type(char *command);

parsed_command_t figure_out_command(char *command_buffer);

void command_parser_task();
