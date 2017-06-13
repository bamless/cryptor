#ifndef THREADPOOL_H
#define THREADPOOL_H

typedef struct ThreadPool ThreadPool; //opaque

ThreadPool* threadpool_create(int thread_count);
void threadpool_destroy(ThreadPool *tp, int flags);
int threadpool_add_task(ThreadPool *tp, void (*task_func)(void *), void *args);

#endif
