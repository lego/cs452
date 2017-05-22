#ifndef __NAMESERVER_H__
#define __NAMESERVER_H__

#define REGISTER_CALL 0
#define WHOIS_CALL 1

typedef struct {
  int call_type;
  char *name;
} nameserver_request_t;

void nameserver();

#endif
