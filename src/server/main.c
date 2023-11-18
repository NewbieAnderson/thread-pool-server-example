#include "server.h"

int main(char *argv[], int argc)
{
    int i;
    int ret;
    int nfds;
    int client_socket;
    struct epoll_event event;
    struct sockaddr_in client_addr;
    struct session *session_ptr = NULL;
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    if (create_server(atoi(argv[1]), 8) == -1) {
        printf("main() - failed to create server.\n");
        goto exit_loop;
    }
    printf("server running at %d port\n", g_server_port);
    while (1) {
        nfds = epoll_wait(g_server_incoming_epfd, g_server_incomming_events, MAX_EPOLL_EVENT_SIZE, -1);
        if (nfds == -1) {
            perror("epoll_wait() failed to waiting socket events ");
            goto exit_loop;
        }
        for (i = 0; i < nfds; ++i) {
            session_ptr = g_server_incomming_events[i].data.ptr;
            if (session_ptr->sockfd == g_server_socket) {
                client_socket = accept(g_server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
                if (client_socket == -1) {
                    perror("failed to create new connection ");
                    goto exit_loop;
                }
                event.data.ptr = create_session_lock(client_socket, &client_addr);
                if (event.data.ptr == NULL) {
                    printf("failed to create new client session\n");
                    goto exit_loop;
                }
                event.events = EPOLLIN;
                if (epoll_ctl(g_server_incoming_epfd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
                    perror("failed to add client's socket ");
                    return -1;
                }
                continue;
            }
            ret = recv_and_push_to_queue_lock(session_ptr->sockfd);
            if (ret == -1) {
                printf("failed to add request to queue\n");
                goto exit_loop;
            } else if (ret == 0) {
                continue;
            }
            if (write(session_ptr->sockfd, "disconn_test\n", strlen("disconn_test\n")) == -1) {
                if (errno == EPIPE || errno == ECONNRESET)
                    printf("client arbitrarily has been shut down connection\n");
                else
                    perror("failed to write towards server ");
                if (delete_session_lock(session_ptr->sockfd) == -1) {
                    printf("failed to delete session\n");
                    goto exit_loop;
                }
            }
        }
        try_wake_up_thread();
    }
exit_loop:
    delete_server();
    return 0;
}