/*
 * TODO : Change this code using array identified by client's socket fd used as index
 */

#ifndef SESSION_BUFFER_H
#define SESSION_BUFFER_H

#include <arpa/inet.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/resource.h>

#include "defs.h"
#include "server.h"

#define INVALID_SOCKET_FD -1

struct session {
    int sockfd;
    unsigned short port;
    u8 ipv4[4];
};

extern struct session g_session_buffer[MAX_CLIENT_SOCKET_SIZE];

static int g_curr_conn_count;
static pthread_t session_manager_thread;
static pthread_mutex_t g_session_buffer_mutex;

void init_session_buffer(void);

void destroy_session_buffer(void);

void clear_full_buffer(void);

struct session *create_session(int sockfd, struct sockaddr_in *addr);

int delete_session(int sockfd);

void *session_manager(void *arg);

#endif