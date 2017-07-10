#include <util/pubsub/interface.h>

typedef struct {

} pubsub_context_t;

void pubsub_register_topic(pubsub_context_t * ctx, message_register_t * msg) {

}

void pubsub_subscribe(pubsub_context_t * ctx, message_subunsub_t * msg) {

}

void pubsub_unsubscribe(pubsub_context_t * ctx, message_subunsub_t * msg) {

}

void pubsub_publish(pubsub_context_t * ctx, message_subunsub_t * msg, void * data) {

}

void pubsub_server() {
  int tid = MyTid();
  int initializer;
  pubsub_init_t init_params;
  ReceiveS(&initializer, init_params);
  ReplyN(initializer);

  pubsub_context_t ctx;

  char buffer[1024];
  int sender;
  message_t *message;

  while (true) {
    int recv_bytes = RecieveS(&sender, buffer);
    message = (message_t *) buffer;
    switch (message->type) {
    case REGISTER_TOPIC_TYPE:
      pubsub_register_topic(&ctx, (message_register_t *) message);
      break;
    case SUBSCRIBE:
      pubsub_subscribe(&ctx, (message_subunsub_t *) message);
      break;
    case UNSUBSCRIBE:
      pubsub_unsubscribe(&ctx, (message_subunsub_t *) message);
      break;
    case PUBLISH:
      pubsub_publish(&ctx, data, (message_publish_t *) message, ((message_publish_t *) message)->data);
      break;
    case TERMINATE:
      // TODO clean up
      return;
      break;
    }
  }
}
