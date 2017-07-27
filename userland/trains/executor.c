#include <worker.h>
#include <kernel.h>
#include <packet.h>
#include <trains/executor.h>
#include <servers/nameserver.h>
#include <servers/clock_server.h>
#include <servers/uart_tx_server.h>
#include <trains/switch_controller.h>
#include <trains/reservoir.h>
#include <trains/navigation.h>
#include <track/pathing.h>
#include <train_command_server.h>
#include <interactive/commands.h>
#include <trains/train_controller.h>
#include <game.h>

// deterministic "random"
int random_counter = 11;
int random_multiplier = 111;

// FIXME: priority
#define SOME_PRIORITY 5

typedef struct {
  // type = PATHING_WORKER_RESULT
  packet_t packet;

  int train;
  int speed;

  pathing_operation_t operation;

  // TODO: maybe make this a heap pointer?
  path_t path;
} pathing_worker_result_t;

/**
 * Fetches the path from the Reservoir and returns it to the Executor
 * @param parent task ID (Executor)
 * @param data   of the navigation request
 */
void pathing_worker(int parent_tid, void * data) {
  cmd_data_t * cmd = (cmd_data_t *) data;
  pathing_worker_result_t result;
  int src_node = WhereAmI(cmd->train);

  int dst = cmd->dest_node;
  if (cmd->base.type == COMMAND_NAVIGATE_RANDOMLY) {
    dst = (random_counter++ * random_multiplier) % 80;
    Logf(EXECUTOR_LOGGING, "Pathing working started for %4s ~> random", track[src_node].name);
  } else {
    Logf(EXECUTOR_LOGGING, "Pathing working started for %4s ~> %4s", track[src_node].name, track[dst].name);
  }

  int status = RequestPath(&result.path, cmd->train, src_node, dst);
  if (result.path.len == 0) {
    Logf(EXECUTOR_LOGGING, "No path found for %4s ~> %4s", track[src_node].name, track[dst].name);
  }
  // FIXME: handle status == -1, i.e. not direct path
  result.packet.type = PATHING_WORKER_RESULT;
  //if (cmd->base.type == COMMAND_NAVIGATE_RANDOMLY) {
  //  while (result.path.dist < 1100) {
  //    dst = (random_counter++ * random_multiplier) % 80;
  //    int status = RequestPath(&result.path, cmd->train, src_node, dst);
  //  }
  //}
  result.train = cmd->train;
  result.speed = cmd->speed;
  switch (cmd->base.type) {
  case COMMAND_NAVIGATE_RANDOMLY:
    result.operation = OPERATION_NAVIGATE_RANDOMLY;
    break;
  case COMMAND_NAVIGATE:
    result.operation = OPERATION_NAVIGATE;
    break;
  case COMMAND_STOP_FROM:
    result.operation = OPERATION_STOPFROM;
    break;
  default:
    KASSERT(false, "Unexpected command type received by pathing worker. Got command=%d", cmd->base.type);
    break;
  }
  // Send result to Executor task
  SendSN(parent_tid, result);
}

/**
 * Executes a CLI command. This is to separate this switch statement of CLI results
 * from the state machine that is the executor
 */
void execute_command(cmd_data_t * cmd_data) {
  path_t p;

  // FIXME: do we want to tell a worker to do these things?
  switch (cmd_data->base.type) {
    case COMMAND_TRAIN_CALIBRATE:
      Logf(EXECUTOR_LOGGING, "Executor got command");
      TellTrainController(cmd_data->train, TRAIN_CONTROLLER_CALIBRATE, 0);
      break;
    case COMMAND_TRAIN_SPEED:
      Logf(EXECUTOR_LOGGING, "Executor got command");
      TellTrainController(cmd_data->train, TRAIN_CONTROLLER_SET_SPEED, cmd_data->speed);
      break;
    case COMMAND_TRAIN_REVERSE:
      TellTrainController(cmd_data->train, TRAIN_CONTROLLER_REVERSE, 0);
      break;
    case COMMAND_SWITCH_TOGGLE:
      SetSwitch(cmd_data->switch_no, cmd_data->switch_dir);
      break;
    case COMMAND_SWITCH_TOGGLE_ALL:
      for (int i = 0; i < NUM_SWITCHES; i++) {
        int switchNumber = i+1;
        if (switchNumber >= 19) {
          switchNumber += 134; // 19 -> 153, etc
        }
        SetSwitch(i, cmd_data->switch_dir);
        // FIXME: don't delay in executor
        Delay(6);
      }
      break;
    case COMMAND_STOP_FROM:
    case COMMAND_NAVIGATE_RANDOMLY:
      Logf(EXECUTOR_LOGGING, "Executor starting random pathing worker for stopfrom or navigate");
      // If we have no starting location, don't do anything (interactive logs this)
      if (WhereAmI(cmd_data->train) == -1) break;
      _CreateWorker(SOME_PRIORITY, pathing_worker, cmd_data, sizeof(cmd_data_t));
      break;
    case COMMAND_NAVIGATE:
      Logf(EXECUTOR_LOGGING, "Executor starting pathing worker for stopfrom or navigate");
      // If we have no starting location, don't do anything (interactive logs this)
      if (WhereAmI(cmd_data->train) == -1) break;
      _CreateWorker(SOME_PRIORITY, pathing_worker, cmd_data, sizeof(cmd_data_t));
      break;
    default:
      KASSERT(false, "Unhandled command send to executor. Got command=%d", cmd_data->base.type);
      break;
  }
}

bool routing_trains[TRAINS_MAX];

void handle_failure(route_failure_t *data) {
  for (int i = 0; i < TRAINS_MAX; i++) {
    if (routing_trains[i])
      TellTrainController(i, TRAIN_CONTROLLER_SET_SPEED, 0);
  }
}

void executor_task() {
  int tid = MyTid();
  RegisterAs(NS_EXECUTOR);

  for (int i = 0; i < TRAINS_MAX; i++) {
    routing_trains[i] = false;
  }

  char request_buffer[1024] __attribute__ ((aligned (4)));
  packet_t * packet = (packet_t *) request_buffer;
  cmd_data_t * cmd = (cmd_data_t *) request_buffer;
  pathing_worker_result_t * pathing_result = (pathing_worker_result_t *) request_buffer;
  route_failure_t *route_failure = (route_failure_t *) request_buffer;
  int sender;
  int game_tid = -1;

  while (true) {
    ReceiveS(&sender, request_buffer);
    ReplyN(sender);
    Logf(EXECUTOR_LOGGING, "Executor got message type=%d", packet->type);
    switch (packet->type) {
    // Command line input invocations
    case INTERPRETED_COMMAND:
      if (cmd->base.type == COMMAND_GAME_START) {
        if (game_tid != -1) Destroy(game_tid);
        game_tid = Create(5, game_task);
        break;
      } else if (cmd->base.type == COMMAND_GAME_TRAIN) {
        station_arrival_t game_init_msg;
        game_init_msg.packet.type = GAME_ADD_TRAIN;
        game_init_msg.train = cmd->train;
        SendSN(game_tid, game_init_msg);
        break;
      }

      execute_command(cmd);
      break;
    case ROUTE_FAILURE:
      handle_failure(route_failure);
      break;
    // Returned pathin result data
    case PATHING_WORKER_RESULT:
      Logf(EXECUTOR_LOGGING, "Executor got pathing worker result");
      routing_trains[pathing_result->train] = true;
      if (pathing_result->operation == OPERATION_NAVIGATE) {
        NavigateTrain(pathing_result->train, pathing_result->speed, &pathing_result->path);
      } else if (pathing_result->operation == OPERATION_NAVIGATE_RANDOMLY) {
        NavigateTrainRandomly(pathing_result->train, pathing_result->speed, &pathing_result->path);
      } else if (pathing_result->operation == OPERATION_STOPFROM) {
        StopTrainAt(pathing_result->train, pathing_result->speed, &pathing_result->path);
      } else {
        KASSERT(false, "Pathing operation not handled. operation=%d", pathing_result->operation);
      }
      break;
    // TODO: anticipated future cases
    // NAVIGATION_FAILURE => when a train's path is interrupted. This is essentially
    //   a request for a new path
    default:
      KASSERT(false, "Got unexpected packet type=%d", packet->type);
      break;
    }
  }
}
