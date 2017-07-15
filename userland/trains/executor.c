#include <worker.h>
#include <packet.h>
#include <trains/executor.h>
#include <servers/nameserver.h>
#include <servers/clock_server.h>
#include <servers/uart_tx_server.h>
#include <trains/switch_controller.h>
#include <trains/route_executor.h>
#include <trains/reservoir.h>
#include <trains/navigation.h>
#include <track/pathing.h>
#include <train_controller.h>
#include <interactive/commands.h>

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
  RecordLogf("Pathing working started\n\r");

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

void execute_command(cmd_data_t * cmd_data) {
  path_t p;

  // FIXME: do we want to tell a worker to do these things?
  switch (cmd_data->base.type) {
    case COMMAND_TRAIN_SPEED:
      SetTrainSpeed(cmd_data->train, cmd_data->speed);
      break;
    case COMMAND_TRAIN_REVERSE:
      ReverseTrain(cmd_data->train, 14);
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
      RecordLogf("Executor starting pathing worker\n\r");
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

void begin_train_controller(pathing_worker_result_t * result) {
  // FIXME: priority
  CreateRouteExecutor(6, result->train, result->speed, &result->path);
}

void executor_task() {
  int tid = MyTid();
  RegisterAs(NS_EXECUTOR);

  char request_buffer[1024] __attribute__ ((aligned (4)));
  packet_t * packet = (packet_t *) request_buffer;
  cmd_data_t * cmd = (cmd_data_t *) request_buffer;
  pathing_worker_result_t * pathing_result = (pathing_worker_result_t *) request_buffer;
  int sender;

  while (true) {
    ReceiveS(&sender, request_buffer);
    ReplyN(sender);
    RecordLogf("Executor got message type=%d\n\r", packet->type);
    switch (packet->type)
     {
      case INTERPRETED_COMMAND:
        execute_command(cmd);
        break;
      case PATHING_WORKER_RESULT:
        RecordLogf("Executor got pathing worker result\n\r");
        begin_train_controller(pathing_result);
        break;
      default:
        KASSERT(false, "Got unexpected packet type=%d", packet->type);
        break;
    }
  }
}
