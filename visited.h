#ifndef VISITED_H
#define VISITED_H

#include <pthread.h>

#define VISITED_BUCKETS 4096

typedef struct vnode{

    char *url;
    struct vnode *next;
} vnode_t;

typedef struct visited{
    vnode_t *buckets[VISITED_BUCKETS];
    pthread_mutex_t lock;
} visited_t;

void visited_init(visited_t *v);
void visited_destroy(visited_t *v);
int visited_try_add(visited_t *v, const char *url);
#endif


