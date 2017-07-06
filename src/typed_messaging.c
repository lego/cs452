#include <kern/typed_messaging.h>

int type_counter = 0;
const char *type_name_table[MAX_TYPES];

typed_msg_t msg_buf;

void _impl_SendTyped(int dest, int *type, const char *type_name, void *data) {
  _impl_EnsureTypeInitialized(type, type_name);
  msg_buf.type = *type;
  msg_buf.data = data;
}

typed_msg_t RecvTyped(int *sender) {
  typed_msg_t msg;
  msg.type = msg_buf.type;
  msg.data = msg_buf.data;
  return msg;
}
