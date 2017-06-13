#ifndef THREADPOOL_H
#define THREADPOOL_H

typedef struct ThreadPool ThreadPool; //opaque

enum shutdown_type {
    /*The worker threads empty the queue before shutting*/
    SOFT_SHUTDOWN = 1,
    /*The worker threads shut down as soon as they can*/
    HARD_SHUTDOWN = 2
};

ThreadPool* threadpool_create(int thread_count);
void threadpool_destroy(ThreadPool *tp, enum shutdown_type type);
int threadpool_add_task(ThreadPool *tp, void (*task_func)(void *, int), void *args);

#endif
