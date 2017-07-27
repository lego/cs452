#include <bwio.h>
#include <idle_task.h>
#include <priorities.h>
#include <track/pathing.h>
#include <trains/navigation.h>
#include <servers/nameserver.h>
#include <servers/uart_tx_server.h>
#include <servers/clock_server.h>
#include <detective/sensor_detector_multiplexer.h>
#include <detective/detector.h>
#include <trains/reservoir.h>
#include <trains/sensor_collector.h>
#include <trains/train_controller.h>
#include <trains/switch_controller.h>
#include <trains/executor.h>
#include <track/pathing.h>
#include <trains/navigation.h>
#include <interactive/commands.h>
#include <kernel.h>
#include <terminal.h>
#include <game.h>

// int ticker = 1;

// void mock_executor() {
//   RegisterAs(NS_EXECUTOR);
//   char buffer[1024] __attribute__((aligned(4)));
//   packet_t *packet = (packet_t *) buffer;
//   route_failure_t *failure = (route_failure_t *) buffer;
//   int sender;
//   while (true) {
//     ReceiveS(&sender, buffer);
//     ReplyN(sender);
//     switch (packet->type) {
//       case ROUTE_FAILURE:
//         bwprintf(COM2, RED_BG "mock_executor" RESET_ATTRIBUTES ": got route failure from %d trying to get to %s\n", failure->train, track[failure->dest_id].name);
//         break;
//     }
//   }
// }

// void ProvideSensorTrigger(int sensor_no) {
//   int tid = WhoIs(NS_SENSOR_ATTRIBUTER);
//   sensor_data_t data;
//   data.packet.type = SENSOR_DATA;
//   data.sensor_no = sensor_no;
//   data.timestamp = ticker++;
//   SendSN(tid, data);
// }

void game_test_task() {
  InitPathing();
  InitNavigation();
  InitTrainControllers();

  Create(PRIORITY_NAMESERVER, nameserver);
  Create(PRIORITY_IDLE_TASK, idle_task);
  Create(PRIORITY_CLOCK_SERVER, clock_server);
  Create(4, uart_tx);

  int game_tid = Create(4, game_task);

  detector_message_t interval_tick;
  interval_tick.packet.type = INTERVAL_DETECT;
  SendSN(game_tid, interval_tick);
  SendSN(game_tid, interval_tick);
  SendSN(game_tid, interval_tick);
  SendSN(game_tid, interval_tick);

  station_arrival_t arrival;
  arrival.packet.type = GAME_ADD_TRAIN;
  arrival.train = 20;
  SendSN(game_tid, arrival);

  arrival.packet.type = GAME_STATION_ARRIVAL;
  arrival.train = 20;
  arrival.station = Name2Node("A4");
  SendSN(game_tid, arrival);

  detector_message_t train_delay;
  train_delay.packet.type = DELAY_DETECT;
  train_delay.details = 20;
  SendSN(game_tid, train_delay);
  SendSN(game_tid, train_delay);
  SendSN(game_tid, train_delay);

  arrival.station = Name2Node("B16");
  SendSN(game_tid, arrival);

  SendSN(game_tid, train_delay);
  SendSN(game_tid, train_delay);
  SendSN(game_tid, train_delay);
  SendSN(game_tid, train_delay);
  SendSN(game_tid, train_delay);


  // Create(PRIORITY_NAMESERVER, nameserver);
  // Create(PRIORITY_IDLE_TASK, idle_task);
  // Create(4, uart_tx);
  // Create(4, sensor_attributer);
  // Create(5, reservoir_task);
  // // Create(6, mock_executor);
  // int executor_tid = Create(6, executor_task);
  // Create(7, switch_controller);
  //
  //
  // int initialSwitchStates[NUM_SWITCHES];
  // initialSwitchStates[ 0] = SWITCH_CURVED;
  // initialSwitchStates[ 1] = SWITCH_CURVED;
  // initialSwitchStates[ 2] = SWITCH_STRAIGHT;
  // initialSwitchStates[ 3] = SWITCH_CURVED;
  // initialSwitchStates[ 4] = SWITCH_CURVED;
  // initialSwitchStates[ 5] = SWITCH_STRAIGHT;
  // initialSwitchStates[ 6] = SWITCH_STRAIGHT;
  // initialSwitchStates[ 7] = SWITCH_STRAIGHT;
  // initialSwitchStates[ 8] = SWITCH_STRAIGHT;
  // initialSwitchStates[ 9] = SWITCH_STRAIGHT;
  // initialSwitchStates[10] = SWITCH_CURVED;
  // initialSwitchStates[11] = SWITCH_STRAIGHT;
  // initialSwitchStates[12] = SWITCH_STRAIGHT;
  // initialSwitchStates[13] = SWITCH_CURVED;
  // initialSwitchStates[14] = SWITCH_CURVED;
  // initialSwitchStates[15] = SWITCH_STRAIGHT;
  // initialSwitchStates[16] = SWITCH_STRAIGHT;
  // initialSwitchStates[17] = SWITCH_CURVED;
  // initialSwitchStates[18] = SWITCH_STRAIGHT;
  // initialSwitchStates[19] = SWITCH_CURVED;
  // initialSwitchStates[20] = SWITCH_STRAIGHT;
  // initialSwitchStates[21] = SWITCH_CURVED;
  // for (int i = 0; i < NUM_SWITCHES; i++) {
  //   int switchNumber = i+1;
  //   if (switchNumber >= 19) {
  //     switchNumber += 134; // 19 -> 153, etc
  //   }
  //   SetSwitch(switchNumber, initialSwitchStates[i]);
  // }
  //
  // TellTrainController(70, TRAIN_CONTROLLER_SET_SPEED, 10);
  //
  // // Train 20 sensors
  // ProvideSensorTrigger(Name2Node("B1"));
  //
  // cmd_data_t data;
  // data.base.packet.type = INTERPRETED_COMMAND;
  // data.base.type = COMMAND_NAVIGATE_RANDOMLY;
  // data.train = 70;
  // data.speed = 10;
  //
  // SendSN(executor_tid, data);
  //
  // // path_t train20_path;
  // // GetPath(&train20_path, Name2Node("D14"), Name2Node("C9"));
  // // PrintPath(&train20_path);
  //
  //
  // // GetPath(&train20_path, Name2Node("D9"), Name2Node("C6"));
  // // // GetPath(&train50_path, Name2Node("C13"), Name2Node("C6"));
  // // PrintPath(&train20_path);
  //
  // //
  // // NavigateTrain(20, 10, &train20_path);
  // //
  // // // Start train 50 (after 20 was attributed)
  // // // NavigateTrain(50, 10, &train50_path);
  // //
  // // // Fails for train 20, keep moving it
  // // ProvideSensorTrigger(Name2Node("E12"));
  // // //
  // // GetPathWithResv(&train50_path, Name2Node("C13"), Name2Node("C16"), 50);
  //
  // // Train 50 sensors
  // // ProvideSensorTrigger(Name2Node("E7"));
  // // ProvideSensorTrigger(Name2Node("D7"));
  // // ProvideSensorTrigger(Name2Node("MR9"));
  //
  // // path_t p;
  // // GetPath(&p, Name2Node("C6"), Name2Node("E5"));
  // // PrintPath(&p);
  // // TellTrainController(71, TRAIN_CONTROLLER_SET_SPEED, 10);
  // //
  // // ProvideSensorTrigger(Name2Node("C6"));
  // // ProvideSensorTrigger(Name2Node("B15"));
  // // ProvideSensorTrigger(Name2Node("A3"));
  // // ProvideSensorTrigger(Name2Node("C11"));
  // // ProvideSensorTrigger(Name2Node("B5"));
  // // ProvideSensorTrigger(Name2Node("D3"));
  // // ProvideSensorTrigger(Name2Node("E5"));

  ExitKernel();
}
