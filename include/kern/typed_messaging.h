#pragma once

#include <stdbool.h>
#include <debug.h>

typedef struct {
  int type;
  void *data;
} typed_msg_t;

#define MessageType(T) T ## _message_typing
#define MessageTypeName(T) T ## _message_typing ## _name
#define DeclMessageType(T) extern int MessageType(T); extern const char * MessageTypeName(T);
#define DefMessageType(T) int MessageType(T) = -1; const char * MessageTypeName(T) = #T;

#define SendTyped(dest, T, data) _impl_SendTyped(dest, &MessageType(T), MessageTypeName(T), &data)
void _impl_SendTyped(int dest, int *type, const char *type_name, void *data);

typed_msg_t RecvTyped(int *sender);

#define IsMessageType(msg, T) ( \
  _impl_EnsureTypeInitialized(&MessageType(T), MessageTypeName(T)), \
  _impl_IsMessageType(&msg, MessageType(T)) \
)

static inline bool _impl_IsMessageType(typed_msg_t *msg, int type) {
  return msg->type == type;
}

#define GetMessageData(msg, T) ( \
 _impl_EnsureTypeInitialized(&MessageType(T), MessageTypeName(T)), \
 _impl_TypeAssertion(&msg, MessageType(T), MessageTypeName(T)), \
 (T *) msg.data \
)

#define GetMessageTypeName(msg) ( \
 type_name_table[msg.type] \
)


#define MAX_TYPES 256
extern const char *type_name_table[MAX_TYPES];

static inline bool _impl_TypeAssertion(typed_msg_t *msg, int type, const char *expected_type_name) {
  KASSERT(_impl_IsMessageType(msg, type), "Incorrect message type casting. Got type=%s expected=%s", type_name_table[msg->type], expected_type_name);
  return false;
}

// for the _impl_EnsureTypeInitialized func to be static inline
extern int type_counter;
static inline bool _impl_EnsureTypeInitialized(int *type, const char *type_name) {
  if (*type == -1) {
    *type = type_counter++;
    KASSERT(*type < MAX_TYPES, "Maximum types reached. Please increase the limit. type_count=%d", *type);
    type_name_table[*type] = type_name;
  }
  return false;
}
