# Message Types

## Goals
Have an easy to use interface to add typing to all messages passed between tasks. Right now there are complexities around sending multiple struct types, and in general it's hard to send more than type of struct. There should be an easy interface to define custom structs and be able to do a switch case of message types and handle them safely.


## Example interfaces

### Send and Receive macros
Use macros to send and receive two separate structs:
- A standard type struct
- A data struct

We can do this by always sending both structs. This requires creating a Send that can copy two pieces of data together, or compacting both structs before calling send -- an inefficient but first attempt.

Challenges in this might be sending the type data, so a way to alleviate those challenges might be to create new implementations of Send and Receive which accommodate sending a type value (int?).


#### Macro-ified interface
```c
my_data_t data;
// do stuff with data
SendTyped(to_tid, data);
data = ReceiveTyped(from_tid);
if (IS_MESSAGE_TYPE(data, my_data_t)) {
  my_data_t actual_data = GET_MESSAGE_DATA(my_data_t, data);
  // do stuff with received data
}
```

#### Non-macro interface
This is here to give guidance to some of the underlying mechanisms. This should not be the interface, as most of it can be simplified down to the above interface which would be ideal.

```c
// create data and type struct
my_data_t data;
msg_type_t msg_type;
msg_type.type = typeof(data); // or some other type enum
char tmp_buffer[sizeof(my_data_t) + sizeof(msg_type_t)];
// compact data into one piece of memory
memcpy(tmp_buffer, msg_type, sizeof(msg_type_t));
memcpy(tmp_buffer + sizeof(msg_type_t), data, sizeof(my_data_t));
// send data
SendSN(tid, tmp_buffer);

// receive data in ambiguous large buffer
char buffer[1024];
ReceiveS(from_tid, buffer);
msg_type_t recv_msg_type = (msg_type_t *) buffer;

if (recv_msg_type->type = typeof(my_data_t))) {
  my_data_t actual_data = (my_data_t *) my_data_t + sizeof(msg_type_t);
  // do stuff
}
```
