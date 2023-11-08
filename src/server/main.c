#include "server.h"

int main(void)
{
    int i;
    int nfds;
    int flag = 0;
    int client_socket;
    const int on = 1;
    struct epoll_event event;
    struct sockaddr_in client_addr;
    struct session *session_ptr = NULL;
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    if (create_server(3000, 8) == -1) {
        printf("main() - failed to create server.\n");
        return -1;
    }
    while (1) {
        nfds = epoll_wait(g_server_incoming_epfd, g_server_incomming_events, MAX_CONNECTION_SIZE, -1);
        if (nfds < 0) {
            perror("epoll_wait() failed to waiting socket events ");
            goto exit_loop;
        }
        for (i = 0; i < nfds; ++i) {
            session_ptr = g_server_incomming_events[i].data.ptr;
            if (session_ptr->sockfd == g_server_socket) {
                // change this socket to nonblocking mode
                client_socket = accept(g_server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
                event.data.ptr = create_new_session(client_socket, &client_addr);
                event.events = EPOLLIN;
                if (epoll_ctl(g_server_incoming_epfd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
                    perror("failed to add client's socket ");
                    return -1;
                }
                continue;
            }
            // NOTICE : TCP "Connection Reset by Peer" error occurs
            // NOTICE : is this code detect TCP disconnection correctly?
            // #2
            if (write(session_ptr->sockfd, "disconn_test\n", strlen("disconn_test\n")) == -1) {
                if (errno == EPIPE) {
                    printf("cient\'s bad pipe\n");
                } else if (errno == EBADF) {
                    printf("client\'s bad file descriptor\n");
                }
                perror("failed to write towards server ");
                close(session_ptr->sockfd);
                // TODO : add this code later!
                // delete_session(session_ptr->sockfd);
                continue;
            }
            // NOTICE : recv_and_push_to_queue() & write() which is first? (#1 vs #2 which is first?)
            // #1
            if (recv_and_push_to_queue(session_ptr->sockfd) == -1) {
                printf("failed to add request to queue\n");
                goto exit_loop;
            }
        }
        try_wake_up_thread();
    }
exit_loop:
    delete_server();
    return 0;
}