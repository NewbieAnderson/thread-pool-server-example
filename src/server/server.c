#include "server.h"

int g_server_port;
int g_server_socket;
int g_server_incoming_epfd;
int g_server_thread_count;
struct sockaddr_in g_server_addr;
struct epoll_event *g_server_incomming_events;
pthread_mutex_t g_server_mutex;

int create_server(int port, int thread_count)
{
    struct epoll_event event;
    g_server_port = port;
    g_server_socket = socket(PF_INET, SOCK_STREAM, 0);
    g_server_incoming_epfd = epoll_create1(0);
    g_server_thread_count = thread_count;
    if (g_server_incoming_epfd < 0) {
        perror("epoll_create1() - failed to create new epoll object ");
        return -1;
    }
    memset(&g_server_addr, 0, sizeof(struct sockaddr_in));
    g_server_port = port;
    g_server_addr.sin_family = AF_INET;
    g_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    g_server_addr.sin_port = htons(port);
    if (bind(g_server_socket, (struct sockaddr *)&g_server_addr, sizeof(struct sockaddr_in)) == -1) {
        perror("bind() - failed to bind server socket ");
        return -1;
    }
    pthread_mutex_init(&g_server_mutex, NULL);
    if (listen(g_server_socket, MAX_WAITING_QUEUE) == -1) {
        perror("listen() - failed to listen server socket");
        return -1;
    }
    g_session = malloc(sizeof(struct session) * g_task_queue_capacity);
    if (g_session == NULL) {
        printf("create_server() - failed to allocate g_session_buffer\n");
        return -1;
    }
    if (init_session_buffer(SESSION_BUFFER_CAPACITY, sizeof(struct session)) == -1) {
        printf("create_server - failed to init session buffer\n");
        return -1;
    }
    event.data.ptr = create_new_session(g_server_socket, &g_server_addr);
    if (event.data.ptr == NULL) {
        printf("create_server() - failed to create new server's session\n");
        return -1;
    }
    event.events = EPOLLIN;
    if (epoll_ctl(g_server_incoming_epfd, EPOLL_CTL_ADD, g_server_socket, &event) == -1) {
        perror("epoll_ctl() - failed to add file descriptor ");
        return -1;
    }
    g_server_incomming_events = malloc(sizeof(struct epoll_event) * MAX_CONNECTION_SIZE);
    if (g_server_incomming_events == NULL) {
        perror("malloc() - failed to allocate epoll_event objects ");
        return -1;
    }
    if (init_thread_pool(g_server_thread_count, MAX_WAITING_QUEUE) == -1) {
        printf("create_server() - failed to init thread pool\n");
        return -1;
    }
    return 0;
}

int delete_server(void)
{
    destroy_thread_pool();
    destroy_session_buffer();
    pthread_mutex_destroy(&g_server_mutex);
    close(g_server_socket);
    return 0;
}