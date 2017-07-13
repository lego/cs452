#pragma once

#include <interactive/interactive.h>

command_t get_command_type(char *command);

interactive_req_t figure_out_command(char *command_buffer);

void command_parser_task();
