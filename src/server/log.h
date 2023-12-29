#ifndef SERVER_H
#define SERVER_H

// NOTICE : IDEA - print some text to console. but, caller DOES NOT PROCESS CONSOLE I/O. instead, other asynchronous i/o manager operate that.

#include <pthread.h>
#include <time.h>

//static pthread_t g_logger_thread;
//static pthread_t g_logger_mutex;
//static struct timeval g_logger_timer;

int init_logger(void);

int delete_logger(void);

int printf_async(const char *formatted_str);

static int logger_main_thread(void);

#endif