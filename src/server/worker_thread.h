#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "defs.h"
#include "session_buffer.h"

#define THREAD_STATE_IS_WORKING 1
#define THREAD_STATE_IS_NOT_WORKING 0

struct task_node {
    int sockfd;
    char recv_buffer[MAX_BUFFER_SIZE];
};

static int g_thread_count;
static int g_task_queue_size;
static int g_task_queue_capacity;
static int g_task_queue_read_ptr;
static int g_task_queue_write_ptr;
static struct task_node *g_task_queue;
static pthread_t *g_threads;
static u8 *g_thread_state;
static pthread_mutex_t g_task_queue_mutex;
static pthread_mutex_t *g_thread_sync_mutex;
static pthread_cond_t *g_thread_sync_cond;

int init_thread_pool(int thread_count, int max_waiting_queue_size);

int destroy_thread_pool(void);

int recv_and_push_to_queue(int sockfd);

int pop_task_queue_copy(struct task_node *task);

int try_wake_up_thread(void);

static void *thread_routine(void *thread_num);

#endif