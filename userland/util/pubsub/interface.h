#pragma once

/**
 * Internal interface implementation
 */

typedef enum {
  REGISTER_TOPIC_TYPE,
  SUBSCRIBE,
  UNSUBSCRIBE,
  PUBLISH,
  TERMINATE,
} type_t;

typedef struct {
  type_t type;
} message_t;

typedef struct {
  message_t base;
  int topic;
  int data_len;
  int sender;
} message_register_t;

typedef struct {
  message_t b
  int topic;
  int sender;
} message_subunsub_t;

typedef struct {
  message_t base;
  int sender;
  int topic;
  int data_len;
  // malloc'd data pointer
  void * data;
} message_publish_t;

typedef struct {
  message_t base;
  int sender;
} message_terminate_t;
