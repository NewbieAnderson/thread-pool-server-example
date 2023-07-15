#include <stdlib.h>

#include "./work_queue.h"

void push_back_task(struct task **root, int client_socket)
{
    struct task *curr_task = *root;
    struct task *new_node = malloc(sizeof(struct task));
    new_node->next = NULL;
    new_node->client_socket = client_socket;
    if (curr_task == NULL) {
        *root = new_node;
        return;
    }
    while (curr_task->next != NULL)
        curr_task = curr_task->next;
    curr_task->next = new_node;
}

struct task *pop_front_task(struct task **root)
{
    struct task *curr_task = *root;
    if (curr_task == NULL)
        return NULL;
    *root = curr_task->next != NULL ? curr_task->next : NULL;
    free(curr_task);
    return curr_task;
}