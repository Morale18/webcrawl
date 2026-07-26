#include <stdlib.h>
#include <string.h>
#include "visited.h"

static unsigned long hash_str(const char *s){

    unsigned long h = 5381;
    int c;
    while((c = *s++)){
        h = ((h<<5) + h ) + (unsigned long)c;
    }
    return h;
}

void visited_init(visited_t *v){
    memset(v, 0, sizeof(*v));
    pthread_mutex_init(&v->lock, NULL);
}

void visited_destroy(visited_t *v){
    for (int i = 0; i < VISITED_BUCKETS; i++){
        vnode_t *n = v->buckets[i];
        while(n){
            vnode_t *next = n->next;
            free(n->url);
            free(n);
            n = next;
        }
    }
    pthread_mutex_destroy(&v->lock);
}

int visited_try_add(visited_t *v, const char *url){
    unsigned long idx = hash_str(url) % VISITED_BUCKETS;

    pthread_mutex_lock(&v->lock);
    for(vnode_t *n = v->buckets[idx]; n ; n = n->next){
      if (strcmp(n->url, url) == 0){
            pthread_mutex_unlock(&v->lock);
            return 0;
        }
    }
    vnode_t *n = malloc(sizeof(*n));
    n->url = strdup(url);
    n->next = v->buckets[idx];
    v->buckets[idx] = n;
    pthread_mutex_unlock(&v->lock);
    return 1;
}


