#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <poll.h>

#define SERVER_PORT 3000
#define MAX_WAITING_QUEUE 256
#define THREAD_STATE_IS_NOT_WORKING 0
#define THREAD_STATE_IS_WORKING 1
#define THREAD_STATE_IS_FINISHED 2

struct server_info {
    struct sockaddr_in server_addr;
    int server_socket;
    int num_of_threads;
    int ip;
    int port;
};

struct thread {
    pthread_t tid;
    pthread_mutex_t sync_mutex;
    pthread_cond_t sync_cond;
    struct sockaddr_in client_addr;
    int client_socket;
    int task_num;
    unsigned char state;
};

struct ui_thread {
    pthread_t tid;
    const struct thread *tasks_ptr;
    int num_of_tasks;
};

void *task_function(void *arg);

#endif