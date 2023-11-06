#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#include <errno.h>
#include <signal.h>

#include "defs.h"
#include "session_buffer.h"

#define THREAD_STATE_IS_WORKING 1
#define THREAD_STATE_IS_NOT_WORKING 0

struct task_node {
    int sockfd;
    char recv_buffer[MAX_BUFFER_SIZE];
};

extern int g_thread_count;
extern pthread_t *g_threads;
extern u8 *g_thread_state;
extern struct task_node *g_task_queue;
extern int g_task_queue_size;
extern int g_task_queue_capacity;
extern int g_task_queue_read_ptr;
extern int g_task_queue_write_ptr;
extern pthread_mutex_t g_task_queue_mutex;
extern pthread_mutex_t *g_task_queue_sync_mutex;
extern pthread_cond_t *g_task_queue_sync_cond;

int init_thread_pool(int thread_count, int max_wating_queue_size);

int destroy_thread_pool(void);

int recv_and_push_to_queue(int sockfd);

int pop_task_queue_copy(struct task_node *task);

// TODO : Implement this function
int try_wake_up_thread(void);

void *thread_routine(void *args);

#endif