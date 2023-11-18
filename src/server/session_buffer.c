#include "session_buffer.h"

void init_session_buffer(void)
{
    int i;
    g_curr_conn_count = 0;
    for (i = 0; i < MAX_CONNECTION_SIZE; ++i)
        g_session_buffer[i].sockfd = INVALID_SOCKET_FD;
    pthread_mutex_init(&g_session_buffer_mutex, NULL);
}

void destroy_session_buffer(void)
{
    clear_full_buffer_lock();
    pthread_mutex_destroy(&g_session_buffer_mutex);
}

void clear_full_buffer_lock(void)
{
    int i;
    pthread_mutex_lock(&g_session_buffer_mutex);
    g_curr_conn_count = 0;
    for (i = 0; i < MAX_CONNECTION_SIZE; ++i)
        g_session_buffer[i].sockfd = INVALID_SOCKET_FD;
    pthread_mutex_unlock(&g_session_buffer_mutex);
}

struct session *create_session_lock(int sockfd, struct sockaddr_in *addr)
{
    int flag;
    if (sockfd == INVALID_SOCKET_FD) {
        printf("create_session() - invalid socket\n");
        return NULL;
    }
    /* NOTICE : if use too many sockets, socket alreay using problem will arise, socket : 0 */
    if (g_session_buffer[sockfd].sockfd != INVALID_SOCKET_FD) {
        printf("create_session() - socket \'%d\' alrealy used, %d\n", sockfd, g_session_buffer[sockfd].sockfd);
        return NULL;
    }
    if (g_curr_conn_count > 0 && g_curr_conn_count >= MAX_CONNECTION_SIZE) {
        printf("create_session() : cannot add more items\n");
        return NULL;
    }
    flag = fcntl(sockfd, F_GETFL, 0);
    if (flag == -1) {
        perror("create_session() - failed to load socket\'s flags ");
        return NULL;
    }
    if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) == -1) {
        perror("create_session() - failed set socket to non-blocking mode ");
        return NULL;
    }
    pthread_mutex_lock(&g_session_buffer_mutex);
    g_session_buffer[sockfd].port = ntohs(addr->sin_port);
    g_session_buffer[sockfd].sockfd = sockfd;
    memcpy(&(g_session_buffer[sockfd].ipv4), &(addr->sin_addr.s_addr), sizeof(unsigned char) * 4);
    pthread_mutex_unlock(&g_session_buffer_mutex);
    return &g_session_buffer[sockfd];
}

/* TODO
 * call this function when connection finished
 * problems may occur when fd that should be used
 * and retrieved again is not retrieved
 */
int delete_session_lock(int sockfd)
{
    int err_code = 1234;
    pthread_mutex_lock(&g_session_buffer_mutex);
    if (g_session_buffer[sockfd].sockfd == INVALID_SOCKET_FD) {
        printf("delete_session() - trying to delete invalid session\n");
        return -1;
    }
retry_close:
    if (close(g_session_buffer[sockfd].sockfd) == -1) {
        err_code = errno;
        pthread_mutex_unlock(&g_session_buffer_mutex);
        perror("delete_session() - error occured on closing socket ");
        if (err_code == EBADF)
            return 0;
        else if (err_code == EINTR)
            goto retry_close;
        return -1;
    }
    g_session_buffer[sockfd].sockfd = INVALID_SOCKET_FD;
    pthread_mutex_unlock(&g_session_buffer_mutex);
    // NOTICE : g_server_mutex not still used before -> not use mutex yet
    // pthread_mutex_lock(&g_server_mutex);
    epoll_ctl(g_server_incoming_epfd, EPOLL_CTL_DEL, g_session_buffer[sockfd].sockfd, NULL);
    // pthread_mutex_unlock(&g_server_mutex);
    return 0;
}

void *session_manager(void *arg)
{
    return NULL;
}