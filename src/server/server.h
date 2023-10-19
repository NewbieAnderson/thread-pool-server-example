#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "defs.h"
#include "worker_thread.h"

extern int g_server_port;
extern int g_server_socket;
extern int g_server_incoming_epfd;
extern int g_server_thread_count;
extern struct sockaddr_in g_server_addr;
extern struct epoll_event *g_server_incomming_events;
extern pthread_mutex_t g_server_mutex;

int create_server(int port, int thread_count);

int delete_server(void);

#endif