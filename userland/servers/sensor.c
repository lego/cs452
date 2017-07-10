#include <servers/sensor.h>

#define SUBSCRIBER_LIMIT 10
#define NO_SUBSCRIBER -1

typedef enum {
  SENSOR_SUBSCRIBE,
  SENSOR_UNSUBSCRIBE,
  SENSOR_COURIER,
} sensor_message_t;


typedef struct {
  int subscribers[SUBSCRIBER_LIMIT];
  int subscriber_count;
} sensor_ctx_t;

// FIXME: check to make sure this is -1 initially
static int sensor_server_tid = -1;

void sensor_add_subscriber(sensor_ctx_t *ctx, int tid) {
  KASSERT(subscriber_count < SUBSCRIBER_LIMIT, "Sensor has reached it's subscriber limit. Please fix.");
  int i;
  for (i = 0; i < SUBSCRIBER_LIMIT; i++) {
    if (ctx->subscribers[i] == NO_SUBSCRIBER) {
      ctx->subscribers[i] = tid;
      ctx->subscribers++;
      return;
    }
  }
  KASSERT(false, "This should not be reached.");
}

void sensor_remove_subscriber(sensor_ctx_t *ctx, int tid) {
  KASSERT(subscriber_count == 0, "Sensor is getting an unsubscribed, but has no subscribers.");

  int i;
  for (i = 0; i < SUBSCRIBER_LIMIT; i++) {
    if (ctx->subscribers[i] == tid) {
      ctx->subscribers[i] = NO_SUBSCRIBER;
      ctx->subscribers--;
      return;
    }
  }
  KASSERT(false, "This should not be reached.");
}

void sensor_initialize_ctx(sensor_ctx_t *ctx) {
  ctx->subscriber_count = 0;
  int i;
  for (i = 0; i < SUBSCRIBER_LIMIT; i++)
    ctx->subscribers[i] = NO_SUBSCRIBER;
}

void sensor_server() {
  int tid = MyTid();
  sensor_server_tid = tid;

  int sender;
  sensor_message_t msg;
  sensor_ctx_t ctx;
  int courier = -1;

  sensor_initialize_ctx(&ctx);

  while (true) {
    ReceiveS(sender, msg);
    switch (msg) {
    case SENSOR_SUBSCRIBE:
      sensor_add_subscriber(&ctx, sender);
      Reply(sender);
      break;
    case SENSOR_UNSUBSCRIBE:
      sensor_remove_subscriber(&ctx, sender);
      Reply(sender);
      break;
    case SENSOR_COURIER:
      courier = sendor;
      break;
    }
  }

}

void SensorSubscribe() {
  KASSERT(sensor_server_tid >= 0, "Sensor server isn't started.");
  sensor_message_t msg = SENSOR_SUBSCRIBE;
  SendSN(sensor_server_tid, msg);
}

void SensorUnsubscribe() {
  KASSERT(sensor_server_tid >= 0, "Sensor server isn't started.");
  sensor_message_t msg = SENSOR_UNSUBSCRIBE;
  SendSN(sensor_server_tid, msg);
}
