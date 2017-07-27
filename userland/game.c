#include <basic.h>
#include <jstring.h>
#include <kernel.h>
#include <cbuffer.h>
#include <servers/nameserver.h>
#include <interactive/commands.h>
#include <servers/uart_tx_server.h>
#include <detective/detector.h>
#include <detective/interval_detector.h>
#include <detective/delay_detector.h>
#include <track/pathing.h>

#include <game.h>

#define MAX_STATIONS 2
#define MAX_TRAINS 4
#define MAX_PIECE_PER_STATION 30
#define MAX_PIECE_PER_TRAIN 10
#define TIME_TO_TRANSFER_PIECE 50
#define TIME_PER_PIECE_SPAWN 400

static unsigned int seed = 1;
void _srand (int newseed) {
  seed = (unsigned)newseed & 0x7fffffffU;
}
int _rand (void) {
  seed = (seed * 1103515245U + 12345U) & 0x7fffffffU;
  return (int)seed;
}

int GetRandom(int end) {
  return _rand() % end;
}

typedef enum {
  CIRCLE,
  SQUARE,
  TRAINGLE,

  // WARNING: keep this at the bottom
  MAX_SHAPE_TYPES,
  INVALID_SHAPE = -1,
} shape_t;

typedef struct {
  shape_t shape;
} piece_t;

struct Train;

typedef struct {
  int id;
  shape_t shape;
  int piece_count;
  bool pieces_used[MAX_PIECE_PER_STATION];
  piece_t pieces[MAX_PIECE_PER_STATION];
  int node;
  struct Train *train;
} station_t;

typedef struct Train {
  int id;
  piece_t pieces[MAX_PIECE_PER_TRAIN];
  bool pieces_used[MAX_PIECE_PER_STATION];
  int piece_count;
  bool unloaded;
  bool loaded;
  station_t *location;
  station_t *destination;
} train_t;

void station_init(station_t *station) {
  station->shape = GetRandom(3);
  station->piece_count = 0;
  for (int i = 0; i < MAX_PIECE_PER_STATION; i++) {
    station->pieces_used[i] = false;
  }
}

void train_init(train_t *train, int id) {
  train->id = id;
  train->destination = (station_t *)0xDEADBEEF;
  train->location = (station_t *)0xDEADBEEF;
  train->piece_count = 0;
  train->unloaded = false;
  train->loaded = false;
  for (int i = 0; i < MAX_PIECE_PER_TRAIN; i++) {
    train->pieces_used[i] = false;
  }
}

void station_add_piece(station_t *station, shape_t shape) {
  for (int i = 0; i < MAX_PIECE_PER_STATION; i++) {
    if (!station->pieces_used[i]) {
      station->pieces_used[i] = true;
      station->pieces[i].shape = shape;
      station->piece_count++;
      return;
    }
  }
  KASSERT(false, "station has too many pieces");
}

shape_t station_remove_piece(station_t *station, shape_t shape) {
  for (int i = 0; i < MAX_PIECE_PER_STATION; i++) {
    if (station->pieces_used[i] && (shape == -1 || station->pieces[i].shape == shape)) {
      station->pieces_used[i] = false;
      station->piece_count--;
      return station->pieces[i].shape;
    }
  }
  KASSERT(false, "Can not remove from empty station");
  return INVALID_SHAPE;
}

void train_add_piece(train_t *train, shape_t shape) {
  for (int i = 0; i < MAX_PIECE_PER_STATION; i++) {
    if (!train->pieces_used[i]) {
      train->pieces_used[i] = true;
      train->pieces[i].shape = shape;
      train->piece_count++;
      return;
    }
  }
  KASSERT(false, "train has too many pieces");
}

shape_t train_remove_piece(train_t *train, shape_t shape) {
  for (int i = 0; i < MAX_PIECE_PER_STATION; i++) {
    if (train->pieces_used[i] && (shape == INVALID_SHAPE || train->pieces[i].shape == shape)) {
      train->pieces_used[i] = false;
      train->piece_count--;
      return train->pieces[i].shape;
    }
  }
  KASSERT(false, "Can not remove from empty train or did not find shape=%d. train.piece_count=%d", shape, train->piece_count);
  return INVALID_SHAPE;
}

station_t *station_for_node(station_t *stations, int node) {
  for (int i = 0; i < MAX_STATIONS; i++) {
    if (stations[i].node == node)
      return &stations[i];
  }
  KASSERT(false, "Arrival for non-station node.");
  return NULL;
}

#define NavTrain(train_no, speed_no, destination) do { \
    navigate_command.train = train_no; \
    navigate_command.speed = speed_no; \
    navigate_command.dest_node = destination; \
    SendSN(executor_tid, navigate_command); \
  } while(0)

void game_task() {
  int station_count = MAX_STATIONS;
  int train_count = 0;
  bool available_shapes[MAX_SHAPE_TYPES];
  station_t stations[MAX_STATIONS];
  train_t trains[MAX_TRAINS];
  int executor_tid = WhoIs(NS_EXECUTOR);

  int station_to_node[MAX_STATIONS];
  station_to_node[0] = Name2Node("A4");
  station_to_node[1] = Name2Node("B16");

  bool train_unloaded = false;
  bool train_loaded = false;
  int sender;

  for (int i = 0; i < MAX_SHAPE_TYPES; i++) {
    available_shapes[i] = false;
  }

  _srand(_rand());

  for (int i = 0; i < MAX_STATIONS; i++) {
    // FIXME: make sure stations are not all the same shape?
    station_t *station = &stations[i];
    station_init(station);
    station->id = i;
    station->node = station_to_node[i];
    available_shapes[station->shape] = true;
    Logf(PACKET_GAME_LOGGING, "Created station=%d shape=%d", station->id, station->shape);
  }

  cmd_data_t navigate_command;
  navigate_command.base.type = COMMAND_NAVIGATE;
  navigate_command.base.packet.type = INTERPRETED_COMMAND;

  char request_buffer[512] __attribute__((aligned(4)));
  packet_t *packet = (packet_t *)request_buffer;
  detector_message_t *detector_msg = (detector_message_t *)request_buffer;
  station_arrival_t *arrival = (station_arrival_t *)request_buffer;

  StartIntervalDetector("game timer", MyTid(), TIME_PER_PIECE_SPAWN);

  while (true) {
    int result = Receive(&sender, request_buffer, sizeof(request_buffer));
    ReplyN(sender);
    switch (packet->type) {
    case INTERVAL_DETECT:
      // Every interval tick spawn a random piece at a random station
      Logf(PACKET_GAME_LOGGING, "1s tick");
      station_t *random_station = &stations[GetRandom(MAX_STATIONS)];
      shape_t random_shape = GetRandom(MAX_SHAPE_TYPES);
      while (random_shape == random_station->shape || !available_shapes[random_shape]) random_shape = GetRandom(MAX_SHAPE_TYPES);
      Logf(PACKET_GAME_LOGGING, "adding shape=%d to station=%d", random_shape, random_station->id);
      station_add_piece(random_station, random_shape);
      break;
    case GAME_ADD_TRAIN:
      // introduce a train into the game, explicitly
      // (we can automate this introduction)
      train_init(&trains[train_count++], arrival->train);
      Logf(PACKET_GAME_LOGGING, "Adding train into game. train=%d train_count=%d", arrival->train, train_count);
      NavTrain(arrival->train, 10, stations[0].node);
      break;
    case GAME_STATION_ARRIVAL: {
      // signal for a train arriving and stopping at a station
      train_t *train = &trains[detector_msg->details];
      train->location = station_for_node(stations, arrival->station);
      // figure out next destination
      // NOTE: determine this possibly from current load that will not be unloaded
      // and the shapes at this station, to find the optimimal place to go to next
      Logf(PACKET_GAME_LOGGING, "train=%d arrived at node %4s station=%d", arrival->train, track[arrival->station].name, train->location->id);
      char buffer[50];
      jformatf(buffer, sizeof(buffer), "transfer delay for train=%d", arrival->train);
      StartRecyclableDelayDetectorWithDetails(buffer, MyTid(), TIME_TO_TRANSFER_PIECE, arrival->train);
      break;
    }
    case DELAY_DETECT: {
      // signal for a delay action of a train stopped at a station
      train_t *train = &trains[detector_msg->details];
      station_t *station = train->location;
      char buffer[50];
      jformatf(buffer, sizeof(buffer), "transfer delay for train=%d", train->id);
      if (!train->unloaded && train->piece_count > 0) {
        // unload pieces from the train that match the station
        train_remove_piece(train, station->shape);
        Logf(PACKET_GAME_LOGGING, "train=%d is unloading shape=%d at station=%d train.piece_count=%d", train->id, station->shape, station->id, train->piece_count);
        if (train->piece_count <= 0) train->unloaded = true;
        StartRecyclableDelayDetectorWithDetails(buffer, MyTid(), TIME_TO_TRANSFER_PIECE, train->id);
      } else if (!train->loaded && station->piece_count > 0) {
        // load pieces from the station to the train
        // ensure we don't trigger the unloading after we load pieces
        train->unloaded = true;
        int shape = station_remove_piece(station, INVALID_SHAPE);
        train_add_piece(train, shape);
        Logf(PACKET_GAME_LOGGING, "train=%d is loading shape=%d at station=%d station.piece_count=%d train.piece_count=%d", train->id, shape, station->id, station->piece_count, train->piece_count);
        if (train->piece_count >= MAX_PIECE_PER_TRAIN || station->piece_count <= 0) train->loaded = true;
        StartRecyclableDelayDetectorWithDetails(buffer, MyTid(), TIME_TO_TRANSFER_PIECE, train->id);
      } else {
        // nav to next destination
        station_t *destination = train->location->id == stations[0].id ? &stations[1] : &stations[0];
        Logf(PACKET_GAME_LOGGING, "train=%d is departing from shape=%d to station=%d (node %4s)", train->id, station->id, destination->id, track[destination->node].name);
        train->loaded = false;
        train->unloaded = false;
        train->destination = destination;
        train->location->train = NULL;
        train->location = NULL;
        NavTrain(train->id, 10, destination->node);
      }
      break;
    }
    default:
      KASSERT(false, "Received invalid packet type=%d result=%d", packet->type, result);
    }
  }

}
