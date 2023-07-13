#include "./server.h"

struct thread *tasks;

void *task_function(void *arg)
{
    struct thread *task = (struct thread *)arg;
    pthread_mutex_t *mutex_ptr = &(task->sync_mutex);
    pthread_cond_t *cond_ptr = &(task->sync_cond);
    while (1) {
        pthread_mutex_lock(mutex_ptr);
        while (task->state == THREAD_STATE_IS_NOT_WORKING)
            pthread_cond_wait(cond_ptr, mutex_ptr);
        task->state = THREAD_STATE_IS_WORKING;
        pthread_mutex_unlock(mutex_ptr);
        write(task->client_socket, "Hello Client!\n", strlen("Hello Client!\n"));
        sleep(10);
        close(task->client_socket);
        pthread_mutex_lock(mutex_ptr);
        task->state = THREAD_STATE_IS_NOT_WORKING;
        pthread_mutex_unlock(mutex_ptr);
    }
    return NULL;
}

void *render_status(void *arg)
{
    struct ui_thread *task = (struct ui_thread *)arg;
    int i;
    while (1) {
        system("clear");
        printf("---------------------------------\n");
        printf("task     state\n");
        for (i = 0; i < task->num_of_tasks; ++i) {
            printf("[#%d]  %s\n",
                   task->tasks_ptr[i].task_num,
                   task->tasks_ptr[i].state == THREAD_STATE_IS_WORKING ? "Working" : "Sleeping");
        }
        printf("---------------------------------\n");
        sleep(1);
    }
    return NULL;
}

int main(void)
{
    struct server_info server;
    struct sockaddr_in client_addr;
    struct ui_thread ui;
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    int client_socket;
    int ret;
    int i;
    server.server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server.server_socket == -1) {
        perror("socket() - failed to create server socket ");
        exit(1);
    }
    server.num_of_threads = 4;
    tasks = malloc(sizeof(struct thread) * server.num_of_threads);
    if (tasks == NULL) {
        printf("malloc() - failed to allocate memory\n");
        exit(1);
    }
    for (i = 0; i < server.num_of_threads; ++i) {
        tasks[i].task_num = i + 1;
        tasks[i].state = THREAD_STATE_IS_NOT_WORKING;
        pthread_mutex_init(&(tasks[i].sync_mutex), NULL);
        pthread_cond_init(&(tasks[i].sync_cond), NULL);
        pthread_create(&(tasks[i].tid), NULL, task_function, &tasks[i]);
    }
    server.port = SERVER_PORT;
    memset(&server.server_addr, 0, sizeof(struct sockaddr_in));
    server.server_addr.sin_family = AF_INET;
    server.server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server.server_addr.sin_port = htons(server.port);
    if (bind(server.server_socket, (struct sockaddr *)&server.server_addr, sizeof(struct sockaddr_in)) == -1) {
        perror("bind() - failed to bind socket ");
        exit(1);
    }
    if (listen(server.server_socket, MAX_WAITING_QUEUE) == -1) {
        perror("listen() - failed to listen socket ");
        exit(1);
    }
    ui.tasks_ptr = tasks;
    ui.num_of_tasks = server.num_of_threads;
    pthread_create(&ui.tid, NULL, render_status, &ui);
    while (1) {
        client_socket = accept(server.server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("failed to accept new client ");
            break;
        }
        for (i = 0; i < server.num_of_threads; ++i) {
            pthread_mutex_lock(&(tasks[i].sync_mutex));
            if (tasks[i].state == THREAD_STATE_IS_WORKING) {
                pthread_mutex_unlock(&(tasks[i].sync_mutex));
                continue;
            }
            tasks[i].client_socket = client_socket;
            tasks[i].client_addr = client_addr;
            tasks[i].state = THREAD_STATE_IS_WORKING;
            pthread_cond_signal(&(tasks[i].sync_cond));
            pthread_mutex_unlock(&(tasks[i].sync_mutex));
            break;
        }
    }
    for (i = 0; i < server.num_of_threads; ++i) {
        pthread_mutex_destroy(&(tasks[i].sync_mutex));
        pthread_cond_destroy(&(tasks[i].sync_cond));
    }
    free(tasks);
    tasks = NULL;
    return 0;
}