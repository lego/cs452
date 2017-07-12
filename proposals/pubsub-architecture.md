# PubSub architecture

## Goals
Create a PubSub style architecture between tasks to enable one task to generate data, and several tasks to conditionally receive it. This can be architected where either the subscriber blocks waiting for data, or it received a message. A helpful result of this would be, for example:

You create a segment of the display to start monitoring track velocities. This subscribes to sensor input, or velocity data, in order to display to the desired part of the display. If we later decide we wish to close this segment and no longer monitor this the task can simply unsub from receiving that data.

## Technical implementation
There are a few important requirements for the pubsub architecture. In particular the pubsub server,
- Needs to not block the data source
- Needs to not block on giving subscribers data

To prevent blocking the data source, the server should only ever be running `Receive`. But to send subscribers data we need to `Send`, so for that we will need an intermediary task of providing subscribers with data once the pubsub server receives it.

An interesting observation is also the possibility of using a single set of PubSub tasks, which all producers pipe into and all consumers subscribe to using an enum of the data type.

Thinking of it this way, producers should communicate to one pubsub task, and consumers should talk to another, and it is up to the architecture implementation to ensure non-blocking properties of each.

## Example interface

### Pubsub server
Note: there is currently no way to have a producer push multiple subscription types. To get around this we may need to add to each push some metadata about the push type. This could possible all be done through a macro, `Push(tid, DataType, data)`.

```c
// get pubsub server
int pubsub_producer = WhoIs(PubSubProducer);


while (true) {
  // get sensor data
  Push(pubsub_producer, SENSOR_DATA, data)
  // get train location data
  Push(pubsub_producer, TRAIN_LOCATION, data)
}
```


### Pubsub client

#### Macro-ified implementation
```c
Subscribe(SENSOR_DATA | TRAIN_LOCATION);

// create a generic buffer data, for multiple pubsub data types
// NOTE: size is max sizeof all possible pubsub reply types
// this could possible be cleaned up by being part of the the Subscribe macro which auto-magically figures out the max size
// it would have to do a one time init in Subscribe
char data_buffer[1024];
pubsub_reply_t *reply;

while (true) {
  reply = GetNotification(data_buffer)
  // NOTE: we could also provide SENSOR_DATA to restrict data notifications or outright remove subscribing
  // we can also do pubsub data type + size checks in GetNotificaton
  // this can implicitly know data_buffer from the note above about defining it in the Subscribe macro.

  // switch on various pubsub subscriptions
  switch (reply->type) {
  case SENSOR_DATA: {
    // pull specific data type, offset by some amount. possibly macro-ify this
    sensor_data_t *data = PULL_FROM_NOTIFICATION(reply, sensor_data_t);
    // do stuff
    break;
  }
  case QUIT: { // some external quit case
    // unsubscribes from SENSOR_DATA
    Unsubscribe(SENSOR_DATA);
    break;
  }
  default:
    // unhandled or bad type
  }

}
```

#### Non-macro implementation
This is here to give guidance to some of the underlying mechanisms. This should not be the interface, as most of it can be simplified down to the above interface which would be ideal.
```c
// get pubsub server
int pubsub_consumer = WhoIs(PubSubConsumer);

// start subscription for SENSOR_DATA
subscribe_t sub;
InitSub(&sub, SENSOR_DATA);
// this could also maybe be (SENSOR_DATA | TRAIN_LOCATION), for multiple simultaneous subscriptions
SendSN(pubsub_consumer, sub);

// create a generic buffer data, for multiple pubsub data types
// NOTE: size is max sizeof all possible pubsub reply types
// this could possible be cleaned up with a macro
char data_buffer[1024];
pubsub_reply_t *reply;

while (true) {
  // get pubsub data
  ReceiveS(&sender, data_buffer);
  ReplyN(sender);
  reply = (pubsub_reply_t *) data_buffer;
  // switch on various pubsub subscriptions
  switch (reply->type) {
  case SENSOR_DATA: {
    // pull specific data type
    sensor_data_t *data = (sensor_data_t *) reply;
    // do stuff
    break;
  }
  case QUIT: { // some external quit case
    // unsubscribes from SENSOR_DATA
    InitUnsub(&sub, SENSOR_DATA);
    SendSN(pubsub_consumer, sub);
    break;
  }
  default:
    // bad type
  }

}
```
