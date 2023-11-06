#include "session_buffer.h"

int g_buf_size;
int g_buf_capacity;
int g_read_ptr;
int g_write_ptr;
struct session *g_session_buffer;
pthread_mutex_t g_session_buffer_mutex;

int init_session_buffer(const int buf_capacity, const int item_byte_size)
{
    if (buf_capacity <= 0)
        return -1;
    g_session_buffer = malloc(sizeof(struct session) * buf_capacity);
    if (g_session_buffer == NULL)
        return -1;
    pthread_mutex_init(&g_session_buffer_mutex, NULL);
    pthread_mutex_lock(&g_session_buffer_mutex);
    g_buf_size = 0;
    g_buf_capacity = buf_capacity;
    g_read_ptr = 0;
    g_write_ptr = 0;
    pthread_mutex_unlock(&g_session_buffer_mutex);
    return 0;
}

int destroy_session_buffer(void)
{
    pthread_mutex_lock(&g_session_buffer_mutex);
    if (g_session_buffer == NULL) {
        return -1;
    }
    free(g_session_buffer);
    g_buf_size = 0;
    g_buf_capacity = 0;
    g_read_ptr = 0;
    g_write_ptr = 0;
    g_session_buffer = NULL;
    pthread_mutex_unlock(&g_session_buffer_mutex);
    pthread_mutex_destroy(&g_session_buffer_mutex);
    return 0;
}

int clear_full_buffer(void)
{
    pthread_mutex_lock(&g_session_buffer_mutex);
    if (g_session_buffer == NULL) {
        return -1;
    }
    g_read_ptr = 0;
    g_write_ptr = 0;
    g_buf_size = 0;
    memset(g_session_buffer, 0, sizeof(struct session) * g_buf_capacity);
    pthread_mutex_unlock(&g_session_buffer_mutex);
    return 0;
}

struct session *create_new_session(int sockfd, struct sockaddr_in *addr)
{
    int flag;
    struct session temp;
    struct session *new_session_ptr;
    u8 ip_bytes[4];
    temp.sockfd = sockfd;
    temp.port = ntohs(addr->sin_port);
    memcpy(&(temp.ipv4), &(addr->sin_addr.s_addr), sizeof(unsigned char) * 4);
    // NOTICE : both client and server sockets are setted into blocking mode
    flag = fcntl(sockfd, F_GETFL, 0);
    if (flag == -1) {
        perror("create_new_session() - failed to load socket\'s falgs ");
        return -1;
    }
    if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) == -1) {
        perror("create_new_session() - failed set socket to non-blocking mode ");
        return -1;
    }
    // NOTICE : same as g_session_buffer[g_write_ptr] = item;
    pthread_mutex_lock(&g_session_buffer_mutex);
    if (g_buf_size > 0 && g_buf_size >= g_buf_capacity) {
        printf("create_new_session() : cannot add more items.\n");
        pthread_mutex_unlock(&g_session_buffer_mutex);
        return NULL;
    }
    memcpy(&(g_session_buffer[g_write_ptr]), &temp, sizeof(struct session));
    new_session_ptr = &(g_session_buffer[g_write_ptr]);
    g_write_ptr = (g_write_ptr + 1) % g_buf_capacity;
    ++g_buf_size;
    pthread_mutex_unlock(&g_session_buffer_mutex);
    return new_session_ptr;
}

int delete_front_session(void)
{
    pthread_mutex_lock(&g_session_buffer_mutex);
    if (g_buf_size <= 0) {
        printf("delete_front_session() : cannot remove more!\n");
        return -1;
    }
    g_session_buffer[g_read_ptr];
    memset(&g_session_buffer[g_read_ptr], 0, sizeof(struct session));
    g_read_ptr = (g_read_ptr + 1) % g_buf_capacity;
    --g_buf_size;
    pthread_mutex_unlock(&g_session_buffer_mutex);
    return 0;
}

int pop_session_buffer_copy(struct session *sess)
{
    if (g_buf_size <= 0)
        return -1;
    pthread_mutex_lock(&g_session_buffer_mutex);
    memcpy(sess, &(g_session_buffer[g_read_ptr]), sizeof(struct session));
    pthread_mutex_unlock(&g_session_buffer_mutex);
    return 0;
}