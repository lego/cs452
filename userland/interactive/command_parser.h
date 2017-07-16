#pragma once

#include <packet.h>
#include <interactive/commands.h>

/**
 * Command Parser is responsible for parsing a full command line string and
 * splicing it into (command enum, argc, argv).
 * Within command_parser is an enumeration of command string => enum
 *
 * Receives from:
 *  UART 2 Rx (Getc)
 *
 * Sends to:
 *   Command Interpreter
 */

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
