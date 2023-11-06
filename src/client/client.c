#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "./client.h"

#define CLIENT_SIZE 100
#define SERVER_PORT 3000
#define IP_STRING "127.0.0.1"

int main(void)
{
    int i;
    int j;
	int cilent_socket[CLIENT_SIZE] = { 0, };
    struct sockaddr_in server_addr;
    const char *str = "hello server!\n";
    const int str_len = strlen(str);
    for (i = 0; i < CLIENT_SIZE; ++i) {
        cilent_socket[i] = socket(PF_INET, SOCK_STREAM, 0);
        if(cilent_socket[i] == -1) {
            perror("failed to create client socket ");
            return -1;
        }
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(IP_STRING);
        server_addr.sin_port = htons(SERVER_PORT);
        if (connect(cilent_socket[i], (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("failed to connect server ");
            return -1;
        }
    }
    for (i = 0; i < 1000; ++i) {
        for (j = 0; j < CLIENT_SIZE; ++j) {
            char send_buf[256] = { 0, };
            snprintf(send_buf, sizeof(send_buf), "i am client %d!\n", cilent_socket[j]);
            if (write(cilent_socket[j], send_buf, sizeof(send_buf)) == -1) {
                printf("[error]\n");
                perror("failed to write bytes to server ");
            }
            sync();
        }
    }
    sleep(60);
    for (i = 0; i < CLIENT_SIZE; ++i)
        close(cilent_socket[i]);
    return 0;
}