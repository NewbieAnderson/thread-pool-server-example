#include "server.h"

int main(void)
{
    int i;
    int nfds;
    int nbytes;
    int client_socket;
    struct epoll_event event;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    if (create_server(3000, 4) == -1) {
        printf("main() - failed to create server.\n");
        return -1;
    }
    while (1) {
        printf("waiting...\n");
        nfds = epoll_wait(g_server_incoming_epfd, g_server_incomming_events, MAX_CONNECTION_SIZE, -1);
        if (nfds < 0) {
            perror("epoll_wait() failed to waiting socket events ");
            goto exit_loop;
        }
        printf("nfds : %d\n", nfds);
        // todo detect tcp disconnection
        for (i = 0; i < nfds; ++i) {
            if (((struct session *)(g_server_incomming_events[i].data.ptr))->sockfd == g_server_socket) {
                client_socket = accept(g_server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
                printf("new connection created!\n");
                event.data.ptr = create_new_session(client_socket, &client_addr);
                event.events = EPOLLIN; // How to detect TCP Disconnection? -> EPOLLHUP | EPOLLERR
                if (epoll_ctl(g_server_incoming_epfd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
                    perror("epoll_ctl() - failed to add client's socket ");
                    return -1;
                }
                if (recv_and_push_to_queue(client_socket) == -1) {
                    printf("main() - failed to add request to queue\n");
                    goto exit_loop;
                }
                continue;
            }
            if (write(((struct session *)(g_server_incomming_events[i].data.ptr))->sockfd, "disconn_test\n", strlen("disconn_test\n")) == -1) {
                printf("tcp disconnected!\n");
                close(((struct session *)(g_server_incomming_events[i].data.ptr))->sockfd);
                continue;
            }
            if (recv_and_push_to_queue(((struct session *)(g_server_incomming_events[i].data.ptr))->sockfd) == -1) {
                printf("main() - failed to add request to queue\n");
                goto exit_loop;
            }
        }
        // TODO : Wake up threads...
        // wake_up_thread();
    }
exit_loop:
    delete_server();
    return 0;
}