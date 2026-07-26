#include <stdlib.h>
#include <string.h>
#include "queue.h"


void queue_init(queue_t *q){
    memset(q, 0, sizeof(*q));
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty,NULL);
    pthread_cond_init(&q->done, NULL);

}

void queue_destroy(queue_t *q){

    task_t *t = q->head;
    while(t){
        task_t *next = t->next;
        free(t->url);
        free(t);
        t = next;
        }
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->done);
}

void queue_push(queue_t *q, const char *url, int depth){
    task_t *t = malloc(sizeof(*t));
    t->url = strdup(url);
    t->depth = depth;
    t->next = NULL;

    pthread_mutex_lock(&q->lock);
    if(q->tail){
        q->tail->next = t;
    }else{
        q->head =t;
    }
    q->tail = t;
    q->size++;
    q->pending++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}


int queue_pop(queue_t *q, char **url, int *depth){
    pthread_mutex_lock(&q->lock);
    while(q->size == 0 && !q->shutdown){

        if(q->pending == 0){
            q->shutdown = 1;
            pthread_cond_broadcast(&q->not_empty);
            pthread_mutex_unlock(&q->lock);
            return -1;
        }
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    if(q->size == 0){
        pthread_mutex_unlock(&q->lock);
        return -1;
    }
    task_t *t = q->head;
    q->head = t->next;
    if(!q->head){
        q->tail = NULL;
    }
    q->size--;

    *url = t->url;
    *depth = t->depth;
    free(t);

    pthread_mutex_unlock(&q->lock);
    return 0;
}


void queue_task_done(queue_t *q){
    pthread_mutex_lock(&q->lock);
    q->pending--;
    if(q->pending == 0 && q-> size == 0){
        q->shutdown = 1;
        pthread_cond_broadcast(&q->not_empty);
        pthread_cond_broadcast(&q->done);
    }
    pthread_mutex_unlock(&q->lock);
}

void queue_wait_until_done(queue_t *q){
    pthread_mutex_lock(&q->lock);
    while(!q->shutdown){
        pthread_cond_wait(&q->done, &q->lock);
    }
    pthread_mutex_unlock(&q->lock);
}










