#ifndef SESSION_BUFFER_H
#define SESSION_BUFFER_H

#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <pthread.h>

#include "defs.h"
#include "server.h"

#define TRY_SESSION_LOCK    pthread_mutex_lock(&g_session_buffer_mutex);
#define TRY_SESSION_UNLOCK  pthread_mutex_unlock(&g_session_buffer_mutex);

struct session {
    int sockfd;
    unsigned short port;
    u8 ipv4[4];
    u8 session_state;
};

extern int g_buf_size;
extern int g_buf_capacity;
extern int g_read_ptr;
extern int g_write_ptr;
// TODO : how about to change session buffer to linked-list?
extern struct session *g_session_buffer;
extern pthread_mutex_t g_session_buffer_mutex;

int init_session_buffer(const int buf_capacity, const int item_byte_size);

int destroy_session_buffer(void);

int clear_full_buffer(void);

struct session *create_new_session(int sockfd, struct sockaddr_in *addr);

int delete_session(int sockfd);

int delete_front_session(void);

int pop_session_buffer_copy(struct session *sess);

#endif