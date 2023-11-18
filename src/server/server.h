#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <unistd.h>

#include "defs.h"
#include "session_buffer.h"
#include "worker_thread.h"

extern int g_server_socket;
extern int g_server_port;
extern int g_server_incoming_epfd;
extern struct epoll_event *g_server_incomming_events;

static pthread_mutex_t g_server_mutex;
static int g_server_thread_count;
static struct sockaddr_in g_server_addr;

int create_server(int port, int thread_count);

int delete_server(void);

#endif