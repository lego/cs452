#include <kernel.h>
#include <jstring.h>
#include <servers/nameserver.h>
#include <trains/switch_controller.h>
#include <track/pathing.h>
#include <interactive/commands.h>
#include <interactive/command_parser.h>
#include <interactive/command_interpreter.h>

#define CMD_ASSERT(a, fmt, ...) do { if (!(a)) { \
    cmd_error_t cmd; cmd.base.packet.type = INTERPRETED_COMMAND; cmd.base.type = COMMAND_INVALID; \
    jformatf(cmd.error, sizeof(cmd.error), fmt, ## __VA_ARGS__); \
    SendSN(interactive_tid, cmd); \
    return; \
  } } while(0)

#define CMD_ASSERT_ARGC(data, expected) CMD_ASSERT(data->argc == expected, "Expected %d arguments, got %d.", expected, data->argc)

#define CMD_ASSERT_IS_INT(data, arg) do { \
    KASSERT(data->argc > arg, "Looks like you forgot a CMD_ASSERT_ARGC assertion above here. Tried to access out of bounds arg %d", arg); \
    int status; jatoui(data->argv[arg], &status); \
    CMD_ASSERT(status == 0, "Expected arg %d to be an int. Got %s", arg+1, data->argv[arg]); \
  } while (0)

#define CMD_ASSERT_IS_TRAIN(data, arg) do { \
  KASSERT(data->argc > arg, "Looks like you forgot a CMD_ASSERT_ARGC assertion above here. Tried to access out of bounds arg %d", arg); \
  CMD_ASSERT_IS_INT(data, arg); \
  int status; int result = jatoui(data->argv[arg], &status); \
  CMD_ASSERT(result > 0 && result < 80, "Expected arg %d to be a train (1-79), got %d.", arg+1, result); \
} while(0)

#define CMD_ASSERT_IS_SPEED(data, arg) do { \
  KASSERT(data->argc > arg, "Looks like you forgot a CMD_ASSERT_ARGC assertion above here. Tried to access out of bounds arg %d", arg); \
  CMD_ASSERT_IS_INT(data, arg); \
  int status; int result = jatoui(data->argv[arg], &status); \
  CMD_ASSERT(result >= 0 && result <= 14, "Expected arg %d to be a speed (0-14), got %d.", arg+1, result); \
} while(0)

#define CMD_ASSERT_IS_SWITCH(data, arg) do { \
  KASSERT(data->argc > arg, "Looks like you forgot a CMD_ASSERT_ARGC assertion above here. Tried to access out of bounds arg %d", arg); \
  CMD_ASSERT_IS_INT(data, arg); \
  int status; int result = jatoui(data->argv[arg], &status); \
  CMD_ASSERT((result > 0 && result <= 18) || (result >= 0x99 && result <= 0x9C), "Expected arg %d to be a switch (1-18, 154-156), got %d.", arg+1, result); \
} while(0)

#define CMD_ASSERT_IS_NODE(data, arg) do { \
  KASSERT(data->argc > arg, "Looks like you forgot a CMD_ASSERT_ARGC assertion above here. Tried to access out of bounds arg %d", arg); \
  int dest_node_id = Name2Node(data->argv[arg]); \
  CMD_ASSERT(dest_node_id != -1, "Expected arg %d to be a node, got %s.", arg+1, data->argv[arg]); \
} while(0)

#define CMD_ASSERT_IS_SWITCH_DIRECTION(data, arg) do { \
  KASSERT(data->argc > arg, "Looks like you forgot a CMD_ASSERT_ARGC assertion above here. Tried to access out of bounds arg %d", arg); \
  CMD_ASSERT(jstrcmp(data->argv[arg], "c") || jstrcmp(data->argv[arg], "C") || jstrcmp(data->argv[arg], "s") || jstrcmp(data->argv[arg], "S"), "Expected arg %d to be a switch direction (S, C), got %s.", arg+1, data->argv[arg]); \
} while(0)

static int GetInt(parsed_command_t * data, int arg) {
  KASSERT(data->argc > arg, "Looks like you forgot a CMD_ASSERT_ARGC assertion above here. Tried to access out of bounds arg %d", arg);
  int status; int result = jatoui(data->argv[arg], &status);
  KASSERT(status == 0, "Looks like you forgot a CMD_ASSERT_IS_INT here. Got argv[%d] = %s which was not an int.", arg, data->argv[arg]);
  return result;
}

static int GetSwitchDirection(parsed_command_t * data, int arg) {
  KASSERT(data->argc > arg, "Looks like you forgot a CMD_ASSERT_ARGC assertion above here. Tried to access out of bounds arg %d", arg);
  if (jstrcmp(data->argv[arg], "c") || jstrcmp(data->argv[arg], "C")) {
    return SWITCH_CURVED;
  } else if (jstrcmp(data->argv[arg], "s") || jstrcmp(data->argv[arg], "S")) {
    return SWITCH_STRAIGHT;
  } else {
    KASSERT(false, "Looks like you forgot a CMD_ASSERT_IS_SWITCH_DIRECTION assertion above here. Got argv[%d] = %s which was not a direction", arg, data->argv[arg]);
    return 0;
  }
}


#define SendToBoth(cmd) do { \
  SendSN(executor_tid, msg); \
  SendSN(interactive_tid, msg); \
  } while(0)


static void command_train_speed(int interactive_tid, parsed_command_t *data) {
  CMD_ASSERT_ARGC(data, 2);
  CMD_ASSERT_IS_TRAIN(data, 0);
  CMD_ASSERT_IS_SPEED(data, 1);
  cmd_data_t msg;
  msg.base.packet.type = INTERPRETED_COMMAND;
  msg.base.type = COMMAND_TRAIN_SPEED;
  msg.train = GetInt(data, 0);
  msg.speed = GetInt(data, 1);
  SendToBoth(msg);
}

static void command_train_reverse(int interactive_tid, parsed_command_t *data) {
  CMD_ASSERT_ARGC(data, 1);
  CMD_ASSERT_IS_TRAIN(data, 0);
  cmd_data_t msg;
  msg.base.packet.type = INTERPRETED_COMMAND;
  msg.base.type = COMMAND_TRAIN_REVERSE;
  msg.train = GetInt(data, 0);
  SendToBoth(msg);
}

static void command_toggle_switch(int interactive_tid, parsed_command_t *data) {
  CMD_ASSERT_ARGC(data, 2);
  CMD_ASSERT_IS_SWITCH(data, 0);
  CMD_ASSERT_IS_SWITCH_DIRECTION(data, 1);
  cmd_data_t msg;
  msg.base.packet.type = INTERPRETED_COMMAND;
  msg.base.type = COMMAND_SWITCH_TOGGLE;
  msg.switch_no = GetInt(data, 0);
  msg.switch_dir = GetSwitchDirection(data, 1);
  SendToBoth(msg);
}

static void command_toggle_all_switches(int interactive_tid, parsed_command_t *data) {
  CMD_ASSERT_ARGC(data, 1);
  CMD_ASSERT_IS_SWITCH_DIRECTION(data, 0);
  cmd_data_t msg;
  msg.base.packet.type = INTERPRETED_COMMAND;
  msg.base.type = COMMAND_SWITCH_TOGGLE_ALL;
  msg.switch_no = -1;
  msg.switch_dir = GetSwitchDirection(data, 0);
  SendToBoth(msg);
}

static void command_navigate(int interactive_tid, parsed_command_t *data) {
  CMD_ASSERT_ARGC(data, 3);
  CMD_ASSERT_IS_TRAIN(data, 0);
  CMD_ASSERT_IS_SPEED(data, 1);
  CMD_ASSERT_IS_NODE(data, 2);
  cmd_data_t msg;
  msg.base.packet.type = INTERPRETED_COMMAND;
  msg.base.type = COMMAND_NAVIGATE;
  msg.train = GetInt(data, 0);
  msg.speed = GetInt(data, 1);
  msg.dest_node = Name2Node(data->argv[2]);
  SendToBoth(msg);
}

static void command_stop_from(int interactive_tid, parsed_command_t *data) {
  CMD_ASSERT_ARGC(data, 3);
  CMD_ASSERT_IS_TRAIN(data, 0);
  CMD_ASSERT_IS_SPEED(data, 1);
  CMD_ASSERT_IS_NODE(data, 2);
  cmd_data_t msg;
  msg.base.packet.type = INTERPRETED_COMMAND;
  msg.base.type = COMMAND_STOP_FROM;
  msg.train = GetInt(data, 0);
  msg.speed = GetInt(data, 1);
  msg.dest_node = Name2Node(data->argv[2]);
  SendToBoth(msg);
}

static void command_path(int interactive_tid, parsed_command_t *data) {
  CMD_ASSERT_ARGC(data, 2);
  CMD_ASSERT_IS_NODE(data, 0);
  CMD_ASSERT_IS_NODE(data, 1);
  cmd_data_t msg;
  msg.base.packet.type = INTERPRETED_COMMAND;
  msg.base.type = COMMAND_PATH;
  msg.src_node = Name2Node(data->argv[0]);
  msg.dest_node = Name2Node(data->argv[1]);
  // NOTE: only send to UI
  SendSN(interactive_tid, msg);
}

static void command_set_velocity(int interactive_tid, parsed_command_t *data) {
  CMD_ASSERT_ARGC(data, 3);
  CMD_ASSERT_IS_TRAIN(data, 0);
  CMD_ASSERT_IS_SPEED(data, 1);
  CMD_ASSERT_IS_INT(data, 2);
  cmd_data_t msg;
  msg.base.packet.type = INTERPRETED_COMMAND;
  msg.base.type = COMMAND_SET_VELOCITY;
  msg.train = GetInt(data, 0);
  msg.speed = GetInt(data, 1);
  msg.extra_arg = GetInt(data, 2);
  // NOTE: only send to UI
  SendSN(interactive_tid, msg);
}

static void command_set_location(int interactive_tid, parsed_command_t *data) {
  CMD_ASSERT_ARGC(data, 2);
  CMD_ASSERT_IS_TRAIN(data, 0);
  CMD_ASSERT_IS_NODE(data, 1);
  cmd_data_t msg;
  msg.base.packet.type = INTERPRETED_COMMAND;
  msg.base.type = COMMAND_SET_LOCATION;
  msg.train = GetInt(data, 0);
  msg.src_node = Name2Node(data->argv[1]);
  // NOTE: only send to UI
  SendSN(interactive_tid, msg);
}

static void command_set_stopping_distance_offset(int interactive_tid, parsed_command_t *data) {
  CMD_ASSERT_ARGC(data, 1);
  CMD_ASSERT_IS_INT(data, 0);
  cmd_data_t msg;
  msg.base.packet.type = INTERPRETED_COMMAND;
  msg.base.type = COMMAND_STOPPING_DISTANCE_OFFSET;
  msg.extra_arg = GetInt(data, 0);
  // NOTE: only send to UI
  SendSN(interactive_tid, msg);
}

static void command_set_stopping_distance(int interactive_tid, parsed_command_t *data) {
  CMD_ASSERT_ARGC(data, 3);
  CMD_ASSERT_IS_TRAIN(data, 0);
  CMD_ASSERT_IS_SPEED(data, 1);
  CMD_ASSERT_IS_INT(data, 2);
  cmd_data_t msg;
  msg.base.packet.type = INTERPRETED_COMMAND;
  msg.base.type = COMMAND_SET_STOPPING_DISTANCE;
  msg.train = GetInt(data, 0);
  msg.speed = GetInt(data, 1);
  msg.extra_arg = GetInt(data, 2);
  // NOTE: only send to UI
  SendSN(interactive_tid, msg);
}

static void command_set_stopping_distanceneg(int interactive_tid, parsed_command_t *data) {
  CMD_ASSERT_ARGC(data, 3);
  CMD_ASSERT_IS_TRAIN(data, 0);
  CMD_ASSERT_IS_SPEED(data, 1);
  CMD_ASSERT_IS_INT(data, 2);
  cmd_data_t msg;
  msg.base.packet.type = INTERPRETED_COMMAND;
  msg.base.type = COMMAND_SET_STOPPING_DISTANCEN;
  msg.train = GetInt(data, 0);
  msg.speed = GetInt(data, 1);
  msg.extra_arg = -GetInt(data, 2);
  // NOTE: only send to UI
  SendSN(interactive_tid, msg);
}

static void command_manual_sense(int interactive_tid, parsed_command_t *data) {
  CMD_ASSERT_ARGC(data, 1);
  CMD_ASSERT_IS_NODE(data, 0);
  cmd_data_t msg;
  msg.base.packet.type = INTERPRETED_COMMAND;
  msg.base.type = COMMAND_MANUAL_SENSE;
  msg.dest_node = GetInt(data, 0);

  if (track[msg.dest_node].type != NODE_SENSOR) {
    cmd_error_t cmd; cmd.base.packet.type = INTERPRETED_COMMAND; cmd.base.type = COMMAND_INVALID;
    jformatf(cmd.error, sizeof(cmd.error), "Expected arg 1 to be a sensor, you provided %s", data->argv[0]);
    SendSN(interactive_tid, cmd);
    return;
  }
  // NOTE: only send to UI
  SendSN(interactive_tid, msg);
}

#define DEF_CASE(cmd, cmd_func) case cmd: cmd_func(interactive_tid, data); break;

void command_interpreter_task() {
  int tid = MyTid();
  RegisterAs(NS_COMMAND_INTERPRETER);
  int interactive_tid = WhoIsEnsured(NS_INTERACTIVE);

  int sender;
  char req_buf[1024];
  parsed_command_t * data = (parsed_command_t *) req_buf;

  cmd_t basic_cmd;
  basic_cmd.packet.type = INTERPRETED_COMMAND;

  cmd_error_t cmd; cmd.base.packet.type = INTERPRETED_COMMAND; cmd.base.type = COMMAND_INVALID;

  while (true) {
    Receive(&sender, data, sizeof(parsed_command_t));
    ReplyN(sender);

    KASSERT(data->packet.type == PARSED_COMMAND, "Unexpected packet type. Expected %d (PARSED_COMMAND) got %d", PARSED_COMMAND, data->packet.type);

    switch (data->type) {
    // All no argument commands, no interpretation needed
    case COMMAND_CLEAR_SENSOR_SAMPLES:
    case COMMAND_CLEAR_SENSOR_OFFSET:
    case COMMAND_PRINT_SENSOR_SAMPLES:
    case COMMAND_PRINT_SENSOR_MULTIPLIERS:
    case COMMAND_PRINT_VELOCITY:
    case COMMAND_QUIT:
      if (data->argc != 0) {
        jformatf(cmd.error, sizeof(cmd.error), "Expected no arguments, got %d.", data->argc);
        SendSN(interactive_tid, cmd);
      } else {
        basic_cmd.type = data->type;
        SendSN(interactive_tid, basic_cmd);
      }
      break;

    DEF_CASE(COMMAND_TRAIN_SPEED, command_train_speed);
    DEF_CASE(COMMAND_TRAIN_REVERSE, command_train_reverse);
    DEF_CASE(COMMAND_SWITCH_TOGGLE, command_toggle_switch);
    DEF_CASE(COMMAND_SWITCH_TOGGLE_ALL, command_toggle_all_switches);
    DEF_CASE(COMMAND_SET_VELOCITY, command_set_velocity);
    DEF_CASE(COMMAND_SET_LOCATION, command_set_location);
    DEF_CASE(COMMAND_STOPPING_DISTANCE_OFFSET, command_set_stopping_distance_offset);
    DEF_CASE(COMMAND_SET_STOPPING_DISTANCE, command_set_stopping_distance);
    DEF_CASE(COMMAND_SET_STOPPING_DISTANCEN, command_set_stopping_distanceneg);
    DEF_CASE(COMMAND_NAVIGATE, command_navigate);
    DEF_CASE(COMMAND_PATH, command_path);
    DEF_CASE(COMMAND_STOP_FROM, command_stop_from);
    DEF_CASE(COMMAND_MANUAL_SENSE, command_manual_sense);

    case COMMAND_INVALID:
      jformatf(cmd.error, sizeof(cmd.error), "Invalid command: %s", data->cmd);
      SendSN(interactive_tid, cmd);
      break;
    }
  }
}
