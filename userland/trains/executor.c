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
#include <trains/route_executor.h>
#include <track/pathing.h>
#include <train_command_server.h>
#include <interactive/commands.h>
#include <trains/train_controller.h>

// FIXME: priority
#define SOME_PRIORITY 5

// FIXME: globals : active_train, active_speed, from interactive.c
extern int active_train;
extern int active_speed;

typedef struct {
  // type = PATHING_WORKER_RESULT
  packet_t packet;

  int train;
  int speed;
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
  Logf(EXECUTOR_LOGGING, "Pathing working started");

  // get the path to BASIS_NODE, our destination point
  int src_node = WhereAmI(cmd->train);
  int status = RequestPath(&result.path, cmd->train, src_node, cmd->dest_node);
  // FIXME: handle status == -1, i.e. not direct path
  result.packet.type = PATHING_WORKER_RESULT;
  result.train = cmd->train;
  result.speed = cmd->speed;
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
    case COMMAND_NAVIGATE:
      Logf(EXECUTOR_LOGGING, "Executor starting pathing worker");
      // If we have no starting location, don't do anything (interactive logs this)
      if (WhereAmI(cmd_data->train) == -1) break;
      _CreateWorker(SOME_PRIORITY, pathing_worker, cmd_data, sizeof(cmd_data_t));
      break;

    case COMMAND_STOP_FROM:
      // FIXME: globals : active_train, active_speed
      if (cmd_data->train != active_train || active_speed <= 0 || WhereAmI(cmd_data->train) == -1) {
        break;
      }
      // get the path to the stopping from node
      GetPath(&p, WhereAmI(cmd_data->train), BASIS_NODE_NAME);
      // set the switches for that route
      SetPathSwitches(&p);
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

  while (true) {
    ReceiveS(&sender, request_buffer);
    ReplyN(sender);
    Logf(EXECUTOR_LOGGING, "Executor got message type=%d", packet->type);
    switch (packet->type) {
    // Command line input invocations
    case INTERPRETED_COMMAND:
      execute_command(cmd);
      break;
    case ROUTE_FAILURE:
      handle_failure(route_failure);
      break;
    // Returned pathin result data
    case PATHING_WORKER_RESULT:
      Logf(EXECUTOR_LOGGING, "Executor got pathing worker result");
      routing_trains[pathing_result->train] = true;
      NavigateTrain(pathing_result->train, pathing_result->speed, &pathing_result->path);
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
