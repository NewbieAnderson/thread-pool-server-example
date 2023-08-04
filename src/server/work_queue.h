#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

struct task {
    struct task *next;
    int client_socket;
};

int push_back_task(struct task **root, int client_socket);

struct task *pop_front_task(struct task **root);

#endif