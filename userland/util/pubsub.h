#pragma once

/**
 * External interface implementation
 */

typedef struct {
  int topic;
  int data_len;
  void * data;
} pubsub_message_t;

typedef struct {
  int priority;
  const char * name;
} pubsub_init_t;

/**
 * Creates a new Pubsub server
 * @return tid
 */
int PubsubCreate(pubsub_init_t * init_params);

void PubsubTerminate(int pubsub_tid);

/**
 * Registers a topic's data type. This helps enforce type checking.
 * NOTE: type checking is currently only done via. size
 * @param  pubsub_tid to register to
 * @param  topic      to register the type as
 * @param  data       to register
 * @return            status
 *                    error if status < 0:
 */
#define RegisterTopicData(pubsub_tid, topic, data) _RegisterTopicDataType(pubsub_tid, topic, sizeof(data))
// NOTE: private interface, hidden by the macro
int _RegisterTopicDataType(int pubsub_tid, int topic, int data_len);

/**
 * Subscribes to data from the pubsub server
 * @param  pubsub_tid of the server to subscribe to
 * @param  topic      to subsribe to
 * @return            status
 *                    error if status < 0:
 *                      ...
 */
int Subscribe(int pubsub_tid, int topic);

/**
 * Unsubscribes from a topic with the pubsub server
 * @param  pubsub_tid to interact with
 * @param  topic      to unsubscribe from
 * @return            status
 *                    error if status < 0:
 *                      ...
 */
int Unsubscribe(int pubsub_tid, int topic);

/**
 * Publish data to all subscribers
 * @param  pubsub_tid to publish to
 * @param  topic      to publish as
 * @param  data
 * @return            status
 *                    error if status < 0:
 *                      ...
 */
#define Publish(publish_tid, topic, data) _Publish(publish_tid, topic, data, sizeof(data));
// NOTE: private interface, hidden by the macro
int _Publish(int pubsub_tid, int topic, void * data, int data_len);

/**
 * Gets the topic from a message
 * @param  pubsub_message to fetch from
 * @return                {int} topic
 */
#define GetTopic(pubsub_message) pubsub_message->topic
