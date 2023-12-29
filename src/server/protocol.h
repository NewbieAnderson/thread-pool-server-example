#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "defs.h"

struct message_header {
};

struct message_body {
};

int parse_message(char *raw_bytes, struct message_header *header_ptr, struct message_body *body_ptr);

#endif