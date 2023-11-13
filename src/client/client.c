#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/resource.h>
#include <sys/socket.h>

#include "./client.h"

#define CLIENT_SIZE 1000
#define REQUEST_TEST_COUNT 3
#define SERVER_PORT 8080
#define SERVER_IP_STRING "127.0.0.1"
#define MAX_CLIENT_SOCKET_SIZE 50000

int main(void)
{
    int i;
    int j;
	int cilent_socket[CLIENT_SIZE] = { 0, };
    char send_buf[256] = { 0, };
    struct sockaddr_in server_addr;
    struct rlimit rlim;
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
    }
    for (i = 0; i < REQUEST_TEST_COUNT; ++i) {
        for (j = 0; j < CLIENT_SIZE; ++j) {
            snprintf(send_buf, sizeof(send_buf), "i am client %d!\n", cilent_socket[j]);
            if (write(cilent_socket[j], send_buf, strlen(send_buf)) == -1) {
                perror("failed to write bytes to server ");
                continue;
            }
            //sync();
        }
    }
    char recv_buf[4096] = { 0, };
    for (j = 0; j < CLIENT_SIZE; ++j) {
        recv(cilent_socket[j], recv_buf, sizeof(recv_buf), 0);
    }
    sleep(3);
    for (i = 0; i < CLIENT_SIZE; ++i)
        close(cilent_socket[i]);
    return 0;
}