#include "worker_thread.h"

int g_thread_count;
pthread_t *g_threads;
pthread_mutex_t g_thread_mutexies;
u8 *g_thread_state;

struct task_node *g_task_queue;
int g_task_queue_size = 0;
int g_task_queue_capacity = 0;
int g_task_queue_read_ptr = 0;
int g_task_queue_write_ptr = 0;
pthread_mutex_t g_task_queue_mutex;

pthread_mutex_t *g_task_queue_sync_mutex;
pthread_cond_t *g_task_queue_sync_cond;

int init_thread_pool(int thread_count, int max_wating_queue_size)
{
    int i;
    int j;
    if (thread_count <= 0 || max_wating_queue_size <= 0) {
        printf("init_thread_pool() - parameter thread_count or max_wating_queue_size is 0 or less than 0");
        return -1;
    }
    g_threads = malloc(sizeof(pthread_t) * thread_count);
    if (g_threads == NULL) {
        printf("init_thread_pool() - failed to allocate g_threads\n");
        return -1;
    }
    pthread_mutex_init(&g_session_buffer_mutex, NULL);
    g_task_queue_size = 0;
    g_task_queue_capacity = max_wating_queue_size;
    g_task_queue_read_ptr = 0;
    g_task_queue_write_ptr = 0;
    g_thread_count = thread_count;
    g_task_queue = malloc(sizeof(struct task_node) * g_thread_count);
    if (g_task_queue == NULL) {
        printf("init_thread_pool() - failed to allocate g_task_queue\n");
        return -1;
    }
    g_task_queue_sync_mutex = malloc(sizeof(u8) * g_thread_count);
    if (g_task_queue_sync_mutex == NULL) {
        printf("init_thread_pool() - failed to allocate g_task_queue_sync_mutex\n");
        return -1;
    }
    g_task_queue_sync_cond = malloc(sizeof(u8) * g_thread_count);
    if (g_task_queue_sync_cond == NULL) {
        printf("init_thread_pool() - failed to allocate g_task_queue_sync_cond\n");
        return -1;
    }
    g_thread_state = malloc(sizeof(u8) * g_thread_count);
    if (g_thread_state == NULL) {
        printf("init_thread_pool() - failed to allocate g_thread_state\n");
        return -1;
    }
    for (i = 0; i < g_thread_count; ++i) {
        if (pthread_create(&g_threads[i], NULL, thread_routine, (void *)i) < 0)
            goto failed;
    }
    return 0;
failed:
    for (j = 0; j < i; ++j) {
        pthread_kill(g_threads[j], SIGKILL);
        pthread_mutex_destroy(&(g_task_queue_sync_mutex[j]));
        pthread_cond_destroy(&(g_task_queue_sync_cond[j]));
    }
    free(g_threads);
    free(g_thread_state);
    return -1;
}

int destroy_thread_pool(void)
{
    int i;
    pthread_mutex_lock(&g_task_queue_mutex);
    if (g_task_queue == NULL) {
        return -1;
    }
    free(g_task_queue);
    g_task_queue_size = 0;
    g_task_queue_capacity = 0;
    g_task_queue_read_ptr = 0;
    g_task_queue_write_ptr = 0;
    g_task_queue = NULL;
    for (i = 0; i < g_thread_count; ++i) {
        if (pthread_kill(g_threads[i], SIGKILL) != 0)
            perror("destroy_thread_pool() - failed to kill thread ");
        pthread_mutex_destroy(&(g_task_queue_sync_mutex[i]));
        pthread_cond_destroy(&(g_task_queue_sync_cond[i]));
    }
    free(g_thread_state);
    g_thread_state = NULL;
    pthread_mutex_unlock(&g_task_queue_mutex);
    pthread_mutex_destroy(&g_task_queue_mutex);
    return 0;
}

// TODO : need to set recive timeout -> 5000ms (5sec)
int recv_and_push_to_queue(int sockfd)
{
    int nbytes;
    pthread_mutex_lock(&g_task_queue_mutex);
    if (g_task_queue_size >= g_task_queue_capacity) {
        printf("read_and_push_to_queue() : cannot add more requests on task queue\n");
        pthread_mutex_unlock(&g_task_queue_mutex);
        return -1;
    }
    nbytes = recv(sockfd, g_task_queue[g_task_queue_write_ptr].recv_buffer, MAX_BUFFER_SIZE, 0);
    if (nbytes == -1) {
        int errnum = errno;
        printf("errno : %d\n", errno);
        if (errno == EPIPE || errno == ECONNRESET) {
            printf("already disconnected by client!\n");
            close(sockfd);
            pthread_mutex_unlock(&g_task_queue_mutex);
            return 0;
        }
        perror("read_and_push_to_queue() - failed to recv from socket ");
        pthread_mutex_unlock(&g_task_queue_mutex);
        return -1;
    } else if (nbytes == 0) {
        printf("tcp connection finished\n");
        close(sockfd);
        return 0;
    }
    printf("recv message : %s\n", g_task_queue[g_task_queue_write_ptr].recv_buffer);
    g_task_queue_write_ptr = (g_task_queue_write_ptr + 1) % g_task_queue_capacity;
    ++g_task_queue_size;
    pthread_mutex_unlock(&g_task_queue_mutex);
    return 0;
}

int pop_task_queue_copy(struct task_node *task)
{
    pthread_mutex_lock(&g_task_queue_mutex);
    if (g_task_queue_size <= 0) {
        printf("pop_task_queue_copy() : cannot remove anymore \n");
        return -1;
    }
    memcpy(task, &g_task_queue[g_task_queue_read_ptr], sizeof(struct task_node));
    g_task_queue_read_ptr = (g_task_queue_read_ptr + 1) % g_task_queue_capacity;
    --g_task_queue_size;
    pthread_mutex_unlock(&g_task_queue_mutex);
    return 0;
}

void *thread_routine(void *args)
{
    struct task_node task;
    int thread_idx = (int)args;
    pthread_mutex_init(&(g_task_queue_sync_mutex[thread_idx]), NULL);
    pthread_cond_init(&(g_task_queue_sync_cond[thread_idx]), NULL);
    g_thread_state[thread_idx] = THREAD_STATE_IS_NOT_WORKING;
    while (1) {
        pthread_mutex_lock(&(g_task_queue_sync_mutex[thread_idx]));
        while (g_thread_state[thread_idx] == THREAD_STATE_IS_NOT_WORKING)
            pthread_cond_wait(&(g_task_queue_sync_cond[thread_idx]), &(g_task_queue_sync_mutex[thread_idx]));
        while (g_buf_size > 0) {
            // process something!
            pop_task_queue_copy(&task);
            printf("message : %s\n", task.recv_buffer);
        }
        g_thread_state[thread_idx] = THREAD_STATE_IS_NOT_WORKING;
        pthread_mutex_unlock(&(g_task_queue_sync_mutex[thread_idx]));
    }
    return NULL;
}