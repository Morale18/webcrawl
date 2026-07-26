
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <pthread.h>
#include <curl/curl.h>

#include "queue.h"
#include "visited.h"
#include "fetch.h"

typedef struct {
    queue_t *queue;
    visited_t *visited;
    int max_depth;
    int worker_id;
} worker_ctx_t;

static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile sig_atomic_t stop_requested = 0;

static void handle_sigint(int sig) {
    (void)sig;
    stop_requested = 1;
}

static void log_line(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    pthread_mutex_lock(&log_lock);
    vprintf(fmt, args);
    fflush(stdout);
    pthread_mutex_unlock(&log_lock);
    va_end(args);
}

typedef struct {
    queue_t *queue;
    visited_t *visited;
    int next_depth;
} link_ctx_t;

static void on_link_found(const char *url, void *ctx) {
    link_ctx_t *lc = ctx;
    if (visited_try_add(lc->visited, url))
        queue_push(lc->queue, url, lc->next_depth);
}

static void *worker_main(void *arg) {
    worker_ctx_t *w = arg;
    CURL *curl = curl_easy_init();
    if (!curl) {
        log_line("[worker %d] failed to init curl handle\n", w->worker_id);
        return NULL;
    }

    char *url;
    int depth;
    while (!stop_requested && queue_pop(w->queue, &url, &depth) == 0) {
        log_line("[worker %d] (depth %d) %s\n", w->worker_id, depth, url);

        buffer_t page;
        if (fetch_url(curl, url, &page) == 0) {
            if (depth < w->max_depth) {
                link_ctx_t lc = { w->queue, w->visited, depth + 1 };
                extract_links(page.data, url, on_link_found, &lc);
            }
            free(page.data);
        } else {
            log_line("[worker %d] FAILED: %s\n", w->worker_id, url);
        }

        free(url);
        queue_task_done(w->queue);
    }

    curl_easy_cleanup(curl);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <seed_url> [num_threads] [max_depth]\n", argv[0]);
        return 1;
    }

    const char *seed = argv[1];
    int num_threads = argc > 2 ? atoi(argv[2]) : 8;
    int max_depth   = argc > 3 ? atoi(argv[3]) : 2;
    if (num_threads < 1) num_threads = 1;

    signal(SIGINT, handle_sigint);
    curl_global_init(CURL_GLOBAL_DEFAULT);

    queue_t queue;
    visited_t visited;
    queue_init(&queue);
    visited_init(&visited);

    visited_try_add(&visited, seed);
    queue_push(&queue, seed, 0);

    pthread_t *threads = malloc(sizeof(pthread_t) * (size_t)num_threads);
    worker_ctx_t *ctxs = malloc(sizeof(worker_ctx_t) * (size_t)num_threads);

    log_line("Starting crawl: seed=%s threads=%d max_depth=%d\n",
              seed, num_threads, max_depth);

    for (int i = 0; i < num_threads; i++) {
        ctxs[i] = (worker_ctx_t){ &queue, &visited, max_depth, i };
        pthread_create(&threads[i], NULL, worker_main, &ctxs[i]);
    }

    for (int i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);

    log_line("Crawl finished.\n");

    free(threads);
    free(ctxs);
    queue_destroy(&queue);
    visited_destroy(&visited);
    curl_global_cleanup();
    return 0;
}

