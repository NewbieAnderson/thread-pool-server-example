#include "server.h"

int g_server_socket;
int g_server_port;
int g_server_incoming_epfd;
struct epoll_event *g_server_incomming_events;

int create_server(int port, int thread_count)
{
    int optval = 1;
    struct rlimit rlim;
    struct epoll_event event = {
        .events = EPOLLIN,
    };
    g_server_port = port;
    g_server_thread_count = thread_count;
    g_server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (g_server_socket == -1) {
        perror("create_server() - failed to create server socket ");
        return -1;
    }
    setsockopt(g_server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    getrlimit(RLIMIT_NOFILE, &rlim);
    rlim.rlim_cur = MAX_CONNECTION_SIZE;
    if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) {
        perror("setrlimit ");
        return 1;
    }
    g_server_incoming_epfd = epoll_create1(0);
    if (g_server_incoming_epfd == -1) {
        perror("create_server() - failed to create new epoll object ");
        return -1;
    }
    memset(&g_server_addr, 0, sizeof(struct sockaddr_in));
    g_server_addr.sin_family = AF_INET;
    g_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    g_server_addr.sin_port = htons(port);
    if (bind(g_server_socket, (struct sockaddr *)&g_server_addr, sizeof(struct sockaddr_in)) == -1) {
        perror("create_server() - failed to bind server socket ");
        return -1;
    }
    pthread_mutex_init(&g_server_mutex, NULL);
    init_session_buffer();
    event.data.ptr = create_session_lock(g_server_socket, &g_server_addr);
    if (event.data.ptr == NULL) {
        printf("create_server() - failed to create new server's session\n");
        return -1;
    }
    if (epoll_ctl(g_server_incoming_epfd, EPOLL_CTL_ADD, g_server_socket, &event) == -1) {
        perror("create_server() - failed to add file descriptor ");
        return -1;
    }
    g_server_incomming_events = malloc(sizeof(struct epoll_event) * MAX_EPOLL_EVENT_SIZE);
    if (g_server_incomming_events == NULL) {
        perror("create_server() - failed to allocate epoll_event objects ");
        return -1;
    }
    if (init_thread_pool(g_server_thread_count, MAX_WAITING_QUEUE_SIZE) == -1) {
        printf("create_server() - failed to init thread pool\n");
        return -1;
    }
    if (listen(g_server_socket, MAX_CONNECTION_SIZE) == -1) {
        perror("create_server() - failed to listen server socket");
        return -1;
    }
    return 0;
}

int delete_server(void)
{
    destroy_thread_pool_lock();
    destroy_session_buffer();
    close(g_server_socket);
    pthread_mutex_destroy(&g_server_mutex);
    free(g_server_incomming_events);
    g_server_incomming_events = NULL;
    return 0;
}