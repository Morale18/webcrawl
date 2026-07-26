#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

typedef struct task{
    char *url;
    int depth;
    struct task *next;
} task_t;

typedef struct queue{
    task_t *head;
    task_t *tail;
    int size;
    int pending;
    int shutdown;
    pthread_mutex_t lock; // one thread at time can access data
    pthread_cond_t not_empty; // wait for specific condition
    pthread_cond_t done;
} queue_t;
void queue_init(queue_t *q);
void queue_destroy(queue_t *q);
void queue_push(queue_t *q, const char *url, int depth);

int queue_pop(queue_t *q, char **url, int *depth);
void queue_task_done(queue_t *q);
void queue_wait_until_done(queue_t *q);

#endif


