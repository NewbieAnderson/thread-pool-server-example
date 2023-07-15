#include "./server.h"

int main(void)
{
    struct thread *threads;
    struct ui_thread ui;
    struct server_info server;
    struct sockaddr_in client_addr;
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
    threads = malloc(sizeof(struct thread) * server.num_of_threads);
    g_task_queue_root = NULL;
    pthread_mutex_init(&g_task_queue_mutex, NULL);
    if (threads == NULL) {
        printf("malloc() - failed to allocate memory\n");
        exit(1);
    }
    for (i = 0; i < server.num_of_threads; ++i) {
        threads[i].state = THREAD_STATE_IS_NOT_WORKING;
        pthread_mutex_init(&(threads[i].sync_mutex), NULL);
        pthread_cond_init(&(threads[i].sync_cond), NULL);
        pthread_create(&(threads[i].tid), NULL, task_function, &threads[i]);
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
    ui.tasks_ptr = threads;
    ui.num_of_threads = server.num_of_threads;
    pthread_create(&ui.tid, NULL, render_status, &ui);
    while (1) {
        client_socket = accept(server.server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("failed to accept new client ");
            break;
        }
        pthread_mutex_lock(&g_task_queue_mutex);
        push_back_task(&g_task_queue_root, client_socket);
        pthread_mutex_unlock(&g_task_queue_mutex);
        for (i = 0; i < server.num_of_threads; ++i) {
            if (threads[i].state == THREAD_STATE_IS_WORKING)
                continue;
            pthread_mutex_lock(&(threads[i].sync_mutex));
            threads[i].state = THREAD_STATE_IS_WORKING;
            pthread_cond_signal(&(threads[i].sync_cond));
            pthread_mutex_unlock(&(threads[i].sync_mutex));
            break;
        }
    }
    for (i = 0; i < server.num_of_threads; ++i) {
        pthread_mutex_destroy(&(threads[i].sync_mutex));
        pthread_cond_destroy(&(threads[i].sync_cond));
    }
    close(server.server_socket);
    free(threads);
    threads = NULL;
    return 0;
}