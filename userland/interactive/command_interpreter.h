#pragma once

#include <packet.h>

/**
 * Command Interpreter is responsible for interpreting (cmd, argc, argv) and
 * turning it into command specific data. Part of this includes full validation
 * which upon failure, this task just alerts interactive of the message to show
 *
 * Receives from:
 *   Command Interpreter
 *
 * Sends to:
 *   Executor
 *   Interpreter
 */

void command_interpreter_task();
