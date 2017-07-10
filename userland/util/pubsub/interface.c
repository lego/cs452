#include <kernel.h>
#include <util/pubsub/server.h>
#include <util/pubsub/interface.h>

int PubsubCreate(pubsub_init_t * init_params) {
  int tid = CreateWithName(init_params->priority, pubsub_server, init_params->name);
  SendSN(tid, init_params);
  return tid;
}

void PubsubTerminate(int pubsub_tid) {
  message_terminate_t msg;
  msg.base.type = TERMINATE;
  msg.sender = active_task->tid; // FIXME: (global reference) active_task
  SendSN(pubsub_tid, msg);
}

int _RegisterTopicDataType(int pubsub_tid, int topic, int data_len) {
  message_register_t msg;
  msg.base.type = REGISTER_TOPIC_TYPE;
  msg.sender = active_task->tid; // FIXME: (global reference) active_task
  msg.topic = topic;
  msg.data_len = data_len;
  SendSN(pubsub_tid, msg);
  return 0;
}

int Subscribe(int pubsub_tid, int topic) {
  message_subunsub_t msg;
  msg.base.type = SUBSCRIBE;
  msg.sender = active_task->tid; // FIXME: (global reference) active_task
  msg.topic = topic;
  SendSN(pubsub_tid, msg);
  return 0;
}

int Unsubscribe(int pubsub_tid, int topic) {
  message_subunsub_t msg;
  msg.base.type = UNSUBSCRIBE;
  msg.sender = active_task->tid; // FIXME: (global reference) active_task
  msg.topic = topic;
  SendSN(pubsub_tid, msg);
  return 0;
}

int _Publish(int pubsub_tid, int topic, void * data, int data_len) {
  message_publish_t msg;
  msg.base.type = PUBLISH;
  msg.sender = active_task->tid; // FIXME: (global reference) active_task
  msg.topic = topic;
  msg.data = data;
  msg.data_len = data_len
  ;
  SendSN(pubsub_tid, msg);
  return 0;
}
