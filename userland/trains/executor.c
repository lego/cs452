#include <trains/executor.h>
#include <server/nameserver.h>
#include <server/clock_server.h>
#include <trains/switch_controller.h>

// FIXME: priority
#define SOME_PRIORITY 5

void begin_navigation(cmd_data_t * cmd) {
  // get the path to BASIS_NODE, our destination point
  GetPath(&p, WhereAmI(cmd_data->train), BASIS_NODE_NAME);
  // set all the switches to go there
  SetPathSwitches(&p);
}

void execute_command(cmd_data_t * cmd) {
  path_t p;

  // FIXME: do we want to tell a worker to do these things?
  switch (cmd->type) {
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
        RenderSwitchChange(i, cmd_data->switch_dir);
        // FIXME: don't delay in executor
        Delay(6);
      }
      break;
    case COMMAND_NAVIGATE:
      CreateWorker(SOME_PRIORITY, navigation_worker, navigation_worker_init, cmd_data);
      break;

    case COMMAND_STOP_FROM:
      if (cmd_data->train != active_train || active_speed <= 0 || WhereAmI(cmd_data->train) == -1) {
        break;
      }
      // get the path to the stopping from node
      GetPath(&p, WhereAmI(cmd_data->train), BASIS_NODE_NAME);
      // set the switches for that route
      SetPathSwitches(&p);
      break;
    default:
      KASSERT(false, "Unhandled command send to executor. Got command=%d", cmd->type);
      break;
  }
}

void executor_task() {
  int tid = MyTid();
  RegisterAs(NS_EXECUTOR);

  char request_buffer[1024];
  packet_t * packet;
  cmd_data_t * cmd;

  while (true) {
    Receive(&sender, request_buffer)
    ReplyN(sender);

    switch (packet->type) {
      case INTERPRETED_COMMAND:
        execute_command(cmd);
        break;
      default:
        KASSERT(false, "Got unexpected packet type=%d", packet->type);
        break;
    }
  }
}
