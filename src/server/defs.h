#ifndef DEFS_H
#define DEFS_H

#define MAX_BUFFER_SIZE 256
#define MAX_CONNECTION_SIZE 163840
#define MAX_EPOLL_EVENT_SIZE 16384
#define MAX_WAITING_QUEUE_SIZE 163840
#define U8_VALIDATE (sizeof(u8) == 1)
#define U32_VALIDATE (sizeof(u32) == 4)

typedef unsigned char u8;
typedef unsigned int u32;

#endif