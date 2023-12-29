#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/epoll.h>

#define CLIENT_SIZE 10000
#define MAX_CLIENT_SOCKET_SIZE 50000
#define TICK_RATE 60
#define SERVER_IP_STRING "127.0.0.1" // "192.168.219.107"
#define SERVER_PORT 8080

long long time_diff_in_ms(struct timespec *start, struct timespec *end)
{
    return ((end->tv_sec - start->tv_sec) * 1000LL + (end->tv_nsec - start->tv_nsec) / 1000000LL);
}

int main(void)
{
    int i;
    int j;
    int epoll_fd;
    int cilent_socket[CLIENT_SIZE] = { 0, };
    char send_buf[256] = { 0, };
    struct epoll_event events[MAX_CLIENT_SOCKET_SIZE];
    struct sockaddr_in server_addr;
    struct timespec start_time;
    struct timespec end_time;
    struct rlimit rlim;
    struct epoll_event event;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP_STRING);
    server_addr.sin_port = htons(SERVER_PORT);
    getrlimit(RLIMIT_NOFILE, &rlim);
    rlim.rlim_cur = MAX_CLIENT_SOCKET_SIZE;
    if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) {
        perror("setrlimit ");
        return 1;
    }
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 ");
        return -1;
    }
    for (i = 0; i < CLIENT_SIZE; ++i) {
        cilent_socket[i] = socket(PF_INET, SOCK_STREAM, 0);
        if (cilent_socket[i] == -1) {
            perror("failed to create client socket ");
            return -1;
        }
        if (connect(cilent_socket[i], (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("failed to connect server ");
            return -1;
        }
        int flag = fcntl(cilent_socket[i], F_GETFL, 0);
        if (flag == -1) {
            perror("failed to load socket\'s flags ");
            return -1;
        }
        if (fcntl(cilent_socket[i], F_SETFL, flag | O_NONBLOCK) == -1) {
            perror("failed set socket to non-blocking mode ");
            return -1;
        }
        event.events = EPOLLIN;
        event.data.fd = cilent_socket[i];
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cilent_socket[i], &event) == -1) {
            perror("epoll_ctl ");
            return -1;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    for (i = 0; i < TICK_RATE; ++i) {
        for (j = 0; j < CLIENT_SIZE; ++j) {
            snprintf(send_buf, sizeof(send_buf), "i am client %d!\n", cilent_socket[j]);
            if (write(cilent_socket[j], send_buf, strlen(send_buf)) == -1) {
                perror("failed to write bytes to server ");
                continue;
            }
        }
    }
    printf("finished call all write()\n");
    char recv_buf[4096] = { 0, };
    while (1) {
        int event_count = epoll_wait(epoll_fd, events, MAX_CLIENT_SOCKET_SIZE, 1000);
        if (event_count == -1) {
            perror("epoll_wait ");
            return -1;
        } else if (event_count == 0) {
            printf("epoll_wait() timeout!\n");
            break;
        }
        for (i = 0; i < event_count; ++i) {
            if (recv(events[i].data.fd, recv_buf, sizeof(recv_buf), 0) == -1)
                perror("failed to recv message ");
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    end_time.tv_sec -= 1;
    printf("Execution time: %lld milliseconds\n", time_diff_in_ms(&start_time, &end_time));
    for (i = 0; i < CLIENT_SIZE; ++i)
        close(cilent_socket[i]);
    printf("finished program!\n");
    return 0;
}