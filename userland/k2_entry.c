#include <basic.h>
#include <bwio.h>
#include <k2_entry.h>
#include <nameserver.h>
#include <kernel.h>
#include <ts7200.h>

////
// From https://stackoverflow.com/a/11946674
static unsigned int seed = 1;
void srand (int newseed) {
  seed = (unsigned)newseed & 0x7fffffffU;
}
int rand (void) {
  seed = (seed * 1103515245U + 12345U) & 0x7fffffffU;
  return (int)seed;
}
////

void _done_send() {
  Send(0, NULL, 0, NULL, 0);
}

void done() {
  Create(31, &_done_send);
}

void waitUntilDone(void* code) {
  bwprintf(COM2, "---\n\r\n\r\n\r");
  Create(0, code);
  int tid; Receive(&tid, NULL, 0); Reply(tid, NULL, 0);
  bwprintf(COM2, "\n\r\n\r---\n\r");
}

void producer() {
  bwprintf(COM2, "P: Registering as PRODUCER_TEST with tid: %d\n\r", MyTid());
  RegisterAs(PRODUCER_TEST);
  bwprintf(COM2, "P: Exit RegisterAs\n\r");

  int source_tid = -1;

  while (true) {
    bwprintf(COM2, "P: Waiting to receive...\n\r");
    int status = Receive(&source_tid, NULL, 0);
    bwprintf(COM2, "P: Received request from %d (status: %d)\n\r", source_tid, status);
    int x = 5;
    Reply(source_tid, &x, sizeof(int));
  }
}

void consumer() {
  bwprintf(COM2, "C: Requesting PRODUCER_TEST tid\n\r");
  int producer_tid = WhoIs(PRODUCER_TEST);
  bwprintf(COM2, "C: Received %d for PRODUCER_TEST\n\r", producer_tid);
  int i;
  for (i = 0; i < 3; i++) {
    int x;
    bwprintf(COM2, "C: Requesting from producer\n\r");
    int status = Send(producer_tid, NULL, 0, &x, sizeof(x));
    bwprintf(COM2, "C: Received %d from producer (status %d)\n\r", x, status);
  }
}

void nameserver_test() {
  Create(3, &consumer);
  Create(2, &producer);

  done();
}

void k2_child_task() {
  int from_tid;
  char buf[100];
  int result;
  char *bye = "BYE";
  bwprintf(COM2, "T1 Child task started.\n\r");
  bwprintf(COM2, "T1 Receiving message.\n\r");
  result = Receive(&from_tid, buf, 100);
  bwprintf(COM2, "T1 Received message. result=%d source_tid=%d msg=%s\n\r", result, from_tid, buf);

  Reply(from_tid, NULL, 0);

  bwprintf(COM2, "T1 Getting parent tid.\n\r");
  int parent_tid = MyParentTid();

  bwprintf(COM2, "T1 Sending message to tid=%d msg=%s\n\r", parent_tid, bye);
  result = Send(parent_tid, bye, 4, NULL, 0);

  bwprintf(COM2, "T1 Child task exiting\n\r");
}

void send_receive_test() {
  int new_task_id;
  int from_tid;
  int result;
  char buf[100];
  char *hello = "HELLO";

  int my_tid = MyTid();

  log_task("Task started", my_tid);
  new_task_id = Create(2, &k2_child_task);
  log_task("Created tid=%d", my_tid, new_task_id);

  log_task("Sending message. tid=%d msg=%s", my_tid, new_task_id, hello);
  result = Send(new_task_id, hello, 6, NULL, 0);
  log_task("Sent message. result=%d to_tid=%d", my_tid, result, new_task_id);

  log_task("Going to receive message.", my_tid);
  result = Receive(&from_tid, buf, 100);
  log_task("Received message. result=%d source_tid=%d msg=%s", my_tid, result, from_tid, buf);
  Reply(from_tid, NULL, 0);

  log_task("Entry task exiting", my_tid);

  done();
}

enum rps_move {
  ROCK, //    0
  PAPER, //   1
  SCISSORS // 2
};

enum rps_result_t {
  WIN,
  LOSE,
  DRAW
}; typedef int rps_result_t;

enum rps_ready_reason_t {
  NEW_PLAYER,
  SOMEONE_LEFT
}; typedef int rps_ready_reason_t;

enum rps_message_type_t {
  // Message types to send to server
  SIGN_UP,
  PLAY,
  QUIT,

  // Message types to send to clients
  READY,
  OK,
  RESULT,
  ERR_INVALID_MOVE,
  ERR_NOT_PLAYING,
  ERR_QUEUE_FULL
}; typedef int rps_message_type_t;

typedef struct {
  rps_message_type_t type;
  int data;
} rps_message_t;

void rps_client() {
  int i;

  int losses = 0;

  int rps_server_tid = WhoIs(RPS_SERVER);

  rps_message_t sendMessage;
  rps_message_t receiveMessage;

  sendMessage.type = SIGN_UP;
  SendS(rps_server_tid, sendMessage, receiveMessage);

  if (receiveMessage.type != READY) {
    Exit();
  }

  while (losses < 3) {
    sendMessage.type = PLAY;
    sendMessage.data = (rand()) % 3;
    SendS(rps_server_tid, sendMessage, receiveMessage);
    if (receiveMessage.type == RESULT && receiveMessage.data == LOSE) {
      losses += 1;
    }
  }

  sendMessage.type = QUIT;
  SendS(rps_server_tid, sendMessage, receiveMessage);
}

void print_move(int player, int move) {
  bwprintf(COM2, "  Player %d plays ", player);
  switch (move) {
    case ROCK:
      bwprintf(COM2, "ROCK");
      break;
    case PAPER:
      bwprintf(COM2, "PAPER");
      break;
    case SCISSORS:
      bwprintf(COM2, "SCISSORS");
      break;
    default:
      bwprintf(COM2, "UNKNOWN");
      break;
  }
  bwprintf(COM2, "\n\r");
}

void rps_server() {
  RegisterAs(RPS_SERVER);

  int i;

  int request_tid;

  int player1_tid = -1;
  int player2_tid = -1;

  int player1_move = -1;
  int player2_move = -1;

  const int play_queue_length = 10;
  int queue_to_play[play_queue_length];
  int queue_start = 0;
  int queue_next = 0;
  for (i = 0; i < play_queue_length; i++) {
    queue_to_play[i] = -1;
  }

  rps_message_t sendMessage;
  rps_message_t receiveMessage;

  while(true) {
    ReceiveS(&request_tid, receiveMessage);
    switch (receiveMessage.type) {
      case SIGN_UP: {
        bool emptyP1 = player1_tid == -1;
        bool emptyP2 = player2_tid == -1;
        if (emptyP1 || emptyP2) {
          bool play = false;
          if (emptyP1) {
            player1_tid = request_tid;
            bwprintf(COM2, "New Player 1 (tid: %d)!\n\r", request_tid);
            play = player2_tid != -1;
          } else {
            player2_tid = request_tid;
            bwprintf(COM2, "New Player 2 (tid: %d)!\n\r", request_tid);
            play = player1_tid != -1;
          }
          if (play) {
            sendMessage.type = READY;
            sendMessage.data = NEW_PLAYER;
            ReplyS(player1_tid, sendMessage);
            ReplyS(player2_tid, sendMessage);
          }
        } else {
          if (queue_next == queue_start && queue_to_play[queue_start] != -1) {
            sendMessage.type = ERR_QUEUE_FULL;
            ReplyS(request_tid, sendMessage);
          } else {
            bwprintf(COM2, "Queueing tid: %d!\n\r", request_tid);
            queue_to_play[queue_next] = request_tid;
            queue_next = (queue_next + 1) % play_queue_length;
          }
          break;
        }
        break;
      }
      case PLAY: {
        bool isP1 = player1_tid == request_tid;
        bool isP2 = player2_tid == request_tid;
        if (isP1 || isP2) {
          if (receiveMessage.data >= 0 && receiveMessage.data <= 2) {
            bool all_moves_in = false;
            if (isP1) {
              player1_move = receiveMessage.data;
              all_moves_in = player2_move != -1;
            } else {
              player2_move = receiveMessage.data;
              all_moves_in = player1_move != -1;
            }
            if (all_moves_in) {
              print_move(1, player1_move);
              print_move(2, player2_move);
              sendMessage.type = RESULT;
              if ((player2_move == 0 && player1_move == 2) || player2_move > player1_move) {
                // Player 2 wins
                bwprintf(COM2, "    -> Player 2 (tid: %d) Wins!\n\r", player2_tid);
                sendMessage.data = LOSE;
                ReplyS(player1_tid, sendMessage);
                sendMessage.data = WIN;
                ReplyS(player2_tid, sendMessage);
              } else if (player1_move == player2_move) {
                // Draw
                bwprintf(COM2, "    -> Draw!\n\r");
                sendMessage.data = DRAW;
                ReplyS(player1_tid, sendMessage);
                ReplyS(player2_tid, sendMessage);
              } else {
                // Player 1 wins
                bwprintf(COM2, "    -> Player 1 (tid: %d) Wins!\n\r", player1_tid);
                sendMessage.data = WIN;
                ReplyS(player1_tid, sendMessage);
                sendMessage.data = LOSE;
                ReplyS(player2_tid, sendMessage);
              }
              // Reset the game
              player1_move = -1;
              player2_move = -1;
              bwgetc(COM2);
            }
          } else {
            sendMessage.type = ERR_INVALID_MOVE;
            ReplyS(request_tid, sendMessage);
          }
        } else {
          sendMessage.type = ERR_NOT_PLAYING;
          ReplyS(request_tid, sendMessage);
        }
        break;
      }
      case QUIT: {
        bool isP1 = player1_tid == request_tid;
        bool isP2 = player2_tid == request_tid;
        if (isP1 || isP2) {
          if (isP1) {
            bwprintf(COM2, "Player 1 Quit (tid: %d)!\n\r", request_tid);
            player1_tid = -1;
          } else {
            bwprintf(COM2, "Player 2 Quit (tid: %d)!\n\r", request_tid);
            player2_tid = -1;
          }
          // If there's stuff in the queue
          if (queue_start != queue_next || queue_to_play[queue_start] != -1) {
            if (isP1) {
              player1_tid = queue_to_play[queue_start];
              bwprintf(COM2, "New Player 1 (tid: %d)!\n\r", queue_to_play[queue_start]);
            } else {
              player2_tid = queue_to_play[queue_start];
              bwprintf(COM2, "New Player 2 (tid: %d)!\n\r", queue_to_play[queue_start]);
            }
            sendMessage.type = READY;
            sendMessage.data = SOMEONE_LEFT;
            ReplyS(queue_to_play[queue_start], sendMessage);
            queue_to_play[queue_start] = -1;
            queue_start = (queue_start + 1) % play_queue_length;
          } else {
            bwprintf(COM2, "Queue Empty, waiting on more players...\n\r", queue_to_play[queue_start]);
            sendMessage.type = OK;
            ReplyS(request_tid, sendMessage);
          }
        } else {
          sendMessage.type = ERR_NOT_PLAYING;
          ReplyS(request_tid, sendMessage);
        }
        break;
      }
    }
  }
}

void play_rps() {
  Create(2, &rps_server);

  int i;
  for (i = 0; i < 3; i++) {
    Create(3, &rps_client);
  }

  for (i = 0; i < 3; i++) {
    Create(4, &rps_client);
  }

  for (i = 0; i < 3; i++) {
    Create(4+2*i, &rps_client);
  }

  done();
}

void k2_entry_task() {
  bwprintf(COM2, "k2_entry\n\r");
  // First things first, make the nameserver
  Create(1, &nameserver);

  waitUntilDone(&send_receive_test);
  waitUntilDone(&nameserver_test);
  waitUntilDone(&play_rps);
}
