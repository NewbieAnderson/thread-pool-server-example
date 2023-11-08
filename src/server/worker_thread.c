#include "worker_thread.h"

int init_thread_pool(int thread_count, int max_wating_queue_size)
{
    int i;
    int j;
    if (thread_count <= 0 || max_wating_queue_size <= 0) {
        printf("init_thread_pool() - parameter \'thread_count\' or \'max_wating_queue_size\' is 0 or negative\n");
        return -1;
    }
    g_thread_count = thread_count;
    g_task_queue_size = 0;
    g_task_queue_capacity = max_wating_queue_size;
    g_task_queue_read_ptr = 0;
    g_task_queue_write_ptr = 0;
    g_task_queue = malloc(sizeof(struct task_node) * g_task_queue_capacity);
    if (g_task_queue == NULL) {
        printf("init_thread_pool() - failed to allocate \'g_task_queue\'\n");
        goto failed_g_task_queue;
    }
    g_threads = malloc(sizeof(pthread_t) * thread_count);
    if (g_threads == NULL) {
        printf("init_thread_pool() - failed to allocate \'g_threads\'\n");
        goto failed_g_threads;
    }
    g_thread_state = malloc(sizeof(u8) * g_thread_count);
    if (g_thread_state == NULL) {
        printf("init_thread_pool() - failed to allocate \'g_thread_state\'\n");
        goto failed_g_thread_state;
    }
    pthread_mutex_init(&g_task_queue_mutex, NULL);
    g_thread_sync_mutex = malloc(sizeof(pthread_mutex_t) * g_thread_count);
    if (g_thread_sync_mutex == NULL) {
        printf("init_thread_pool() - failed to allocate \'g_task_queue_sync_mutex\'\n");
        goto failed_g_thread_sync_mutex;
    }
    g_thread_sync_cond = malloc(sizeof(pthread_cond_t) * g_thread_count);
    if (g_thread_sync_cond == NULL) {
        printf("init_thread_pool() - failed to allocate \'g_task_queue_sync_cond\'\n");
        goto failed_g_thread_sync_cond;
    }
    for (i = 0; i < g_thread_count; ++i) {
        if (pthread_create(&g_threads[i], NULL, thread_routine, (void *)i) < 0) {
            printf("init_thread_pool() - failed to create thread %d\n", i + 1);
            goto failed_create_thread;

        }
    }
    return 0;
failed_create_thread:
    for (j = 0; j < i; ++j) {
        pthread_kill(g_threads[j], SIGKILL);
        pthread_mutex_destroy(&(g_thread_sync_mutex[j]));
        pthread_cond_destroy(&(g_thread_sync_cond[j]));
    }
    free(g_thread_sync_cond);
failed_g_thread_sync_cond:
    free(g_thread_sync_mutex);
failed_g_thread_sync_mutex:
    free(g_thread_state);
failed_g_thread_state:
    free(g_threads);
failed_g_threads:
    free(g_task_queue);
failed_g_task_queue:
    g_task_queue = NULL;
    g_threads = NULL;
    g_thread_state = NULL;
    g_thread_sync_mutex = NULL;
    g_thread_sync_cond = NULL;
    return -1;
}

int destroy_thread_pool(void)
{
    int i;
    pthread_mutex_lock(&g_task_queue_mutex);
    if (g_task_queue == NULL) {
        printf("destroy_thread_pool() - trying to delete null thread pool\n");
        return -1;
    }
    free(g_task_queue);
    free(g_threads);
    free(g_thread_state);
    g_thread_count = 0;
    g_task_queue_size = 0;
    g_task_queue_capacity = 0;
    g_task_queue_read_ptr = 0;
    g_task_queue_write_ptr = 0;
    g_task_queue = NULL;
    g_threads = NULL;
    g_thread_state = NULL;
    for (i = 0; i < g_thread_count; ++i) {
        if (pthread_kill(g_threads[i], SIGKILL) != 0)
            perror("destroy_thread_pool() - failed to kill thread ");
        pthread_mutex_destroy(&(g_thread_sync_mutex[i]));
        pthread_cond_destroy(&(g_thread_sync_cond[i]));
    }
    pthread_mutex_unlock(&g_task_queue_mutex);
    pthread_mutex_destroy(&g_task_queue_mutex);
    return 0;
}

int recv_and_push_to_queue(int sockfd)
{
    int nbytes;
    if (g_task_queue_size >= g_task_queue_capacity) {
        printf("read_and_push_to_queue() - cannot add more requests on task queue\n");
        return 0;
    }
    pthread_mutex_lock(&g_task_queue_mutex);
    nbytes = recv(sockfd, g_task_queue[g_task_queue_write_ptr].recv_buffer, MAX_BUFFER_SIZE, 0);
    if (nbytes == 0) {
        pthread_mutex_unlock(&g_task_queue_mutex);
        close(sockfd);
        printf("recv_and_push_to_queue() - read 0 bytes, client disconnected\n");
        return 0;
    } else if (nbytes == -1) {
        perror("recv_and_push_to_queue() - failed to read bytes ");
        pthread_mutex_unlock(&g_task_queue_mutex);
        close(sockfd);
        return -1;
    }
    g_task_queue_write_ptr = (g_task_queue_write_ptr + 1) % g_task_queue_capacity;
    ++g_task_queue_size;
    pthread_mutex_unlock(&g_task_queue_mutex);
    return 0;
}

int pop_task_queue_copy(struct task_node *task)
{
    if (g_task_queue_size <= 0) {
        printf("pop_task_queue_copy() - cannot remove anymore\n");
        return 0;
    }
    if (task == NULL) {
        printf("pop_task_queue_copy() - invalid parameter \'task\'\n");
        return -1;
    }
    pthread_mutex_lock(&g_task_queue_mutex);
    memcpy(task, &g_task_queue[g_task_queue_read_ptr], sizeof(struct task_node));
    g_task_queue_read_ptr = (g_task_queue_read_ptr + 1) % g_task_queue_capacity;
    --g_task_queue_size;
    pthread_mutex_unlock(&g_task_queue_mutex);
    return 0;
}

int try_wake_up_thread(void)
{
    int i;
    for (i = 0; i < g_thread_count; ++i) {
        if (g_thread_state[i] == THREAD_STATE_IS_WORKING)
            continue;
        g_thread_state[i] = THREAD_STATE_IS_WORKING;
        pthread_cond_signal(&(g_thread_sync_cond[i]));
    }
    return 0;
}

static void *thread_routine(void *thread_num)
{
    struct task_node task = {
        .sockfd = -1,
    };
    int thread_idx = (int)thread_num;
    g_thread_state[thread_idx] = THREAD_STATE_IS_NOT_WORKING;
    pthread_mutex_init(&(g_thread_sync_mutex[thread_idx]), NULL);
    pthread_cond_init(&(g_thread_sync_cond[thread_idx]), NULL);
    while (1) {
        pthread_mutex_lock(&(g_thread_sync_mutex[thread_idx]));
        while (g_thread_state[thread_idx] == THREAD_STATE_IS_NOT_WORKING)
            pthread_cond_wait(&(g_thread_sync_cond[thread_idx]), &(g_thread_sync_mutex[thread_idx]));
        pthread_mutex_unlock(&(g_thread_sync_mutex[thread_idx]));
        while (g_task_queue_size > 0) {
            pop_task_queue_copy(&task);
            if (task.sockfd == -1)
                continue;
            printf("thread #%d, message : %s\n", thread_idx + 1, task.recv_buffer);
            task.sockfd = -1;
        }
        g_thread_state[thread_idx] = THREAD_STATE_IS_NOT_WORKING;
    }
    return NULL;
}