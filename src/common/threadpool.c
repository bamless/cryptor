#include "threadpool.h"
#include "thread.h"
#include "logging.h"

typedef struct ThreadPoolTask {
    struct ThreadPoolTask *next;
    void (*task_func)(void *);
    void *args;
} ThreadPoolTask;

struct ThreadPool {
    int thread_count;
    int queue_size;
    ThreadPoolTask *tasks_queue;
    int shutting;
    Thread *threads;
    Mutex tp_lock;
    CondVar tasks_cond;
};

typedef struct wthread_arg {
    ThreadPool *tp;
    int id;
};

static void init_threads(Thread *, ThreadPool *);
static void worker_thread(void *);

ThreadPool* threadpool_create(int thread_count) {
    ThreadPool *tp = malloc(sizeof(ThreadPool));
    if(!tp) return NULL;
    tp->thread_count = thread_count;
    tp->queue_size = 0;
    tp->tasks_queue = NULL;
    tp->shutting = 0;
    tp->threads = malloc(sizeof(Thread) * thread_count);
    if(!tp->threads) {
        free(tp);
        return NULL;
    }
    thread_init_cond(&tp->tasks_cond);
    thread_init_mutex(&tp->tp_lock);
    //start the worker threads
    init_threads(tp->threads, ThreadPool *tp);
}

static void init_threads(Thread *threads, ThreadPool *tp) {
    dlog("Initializing threads...");
    for(int i = 0; i < tp->thread_count; i++) {
        struct wthread_arg *wtarg = malloc(sizeof(struct wthread_arg));
        wtarg->tp = tp;
        wtarg->id = i;
        dlogf("%s %d", "Initializing thread", i);
        thread_create(&threads[i], &worker_thread, wtarg);
    }
}

//TODO: implement shutting flags
void threadpool_destroy(ThreadPool *tp, int flags) {
    //set the shutting flag for the threads (we need to synchronize)
    thread_lock_mutex(tp->tp_lock);
    //we are already shutting
    if(tp->shutting) {
        thread_unlock_mutex(tp->tp_lock);
        return;
    }
    tp->shutting = 1;
    thread_cond_signal_all(tp->tasks_cond); //wake waiting threads
    thread_unlock_mutex(tp->tp_lock);

    //wait for all the threads to finish
    dlog("Waiting for the threads to shut down...");
    for(int i = 0; i < tp->thread_count; i++)
        thread_join(&tp->threads[i]);

    //free all the resources
    thread_destroy_cond(tp->tasks_cond);
    thread_destroy_mutex(tp->tp_lock);
    free(tp->threads);
    free(tp);
}

static void worker_thread(void *worker_thread_arg) {
    struct wthread_arg *wtarg = worker_thread_arg;

    ThreadPool *threadpool = wtarg->tp;
    int thread_id = wtarg->id;

    free(wtarg);

    for(;;) {

    }
}
