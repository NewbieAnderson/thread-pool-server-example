#include "./server.h"

void *task_function(void *arg) {
    struct thread *args = (struct thread *)arg;
    int client_socket = -1;
    pthread_mutex_t *mutex_ptr = &(args->sync_mutex);
    pthread_cond_t *cond_ptr = &(args->sync_cond);
    while (1) {
        pthread_mutex_lock(mutex_ptr);
        while (args->state == THREAD_STATE_IS_NOT_WORKING)
            pthread_cond_wait(cond_ptr, mutex_ptr);
        pthread_mutex_unlock(mutex_ptr);
        while (g_task_queue_root != NULL) {
            pthread_mutex_lock(&g_task_queue_mutex);
            if (g_task_queue_root != NULL) {
                client_socket = g_task_queue_root->client_socket;
                pop_front_task(&g_task_queue_root);
                pthread_mutex_unlock(&g_task_queue_mutex);
                // TODO : Create new session
                write(client_socket, "Hello Client!\n", strlen("Hello Client!\n"));
                sleep(10);
                close(client_socket);
            } else {
                pthread_mutex_unlock(&g_task_queue_mutex);
            }
        }
        pthread_mutex_lock(mutex_ptr);
        args->state = THREAD_STATE_IS_NOT_WORKING;
        pthread_mutex_unlock(mutex_ptr);
    }
    return NULL;
}

void *render_status(void *arg)
{
    struct ui_thread *args = (struct ui_thread *)arg;
    int i;
    while (1) {
        system("clear");
        printf("-----------------------------------\n"
                "Task  State   \n");
        for (i = 0; i < args->num_of_threads; ++i) {
            printf("[#%d]  %s\n",
                   i + 1,
                   args->tasks_ptr[i].state == THREAD_STATE_IS_WORKING ? "Working " : "Sleeping");
        }
        printf("-----------------------------------\n");
        sleep(1);
    }
    return NULL;
}