#include "threadpool.h"
#include "thread.h"
#include "logging.h"

typedef struct ThreadPoolTask {
    struct ThreadPoolTask *next;
    void (*task_func)(void *, int);
    void *args;
} ThreadPoolTask;

struct ThreadPool {
    int thread_count;
    int queue_size;
    ThreadPoolTask *tasks_head;
    ThreadPoolTask *tasks_tail;
    int shutting;
    Thread *threads;
    Mutex tp_lock;
    CondVar tasks_cond;
};

struct wthread_arg {
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
    tp->tasks_head = NULL;
    tp->tasks_tail = NULL;
    tp->shutting = 0;
    tp->threads = malloc(sizeof(Thread) * thread_count);
    if(!tp->threads) {
        free(tp);
        return NULL;
    }
    thread_init_cond(&tp->tasks_cond);
    thread_init_mutex(&tp->tp_lock);
    //start the worker threads
    init_threads(tp->threads, tp);
    return tp;
}

static void init_threads(Thread *threads, ThreadPool *tp) {
    thread_lock_mutex(&tp->tp_lock);
    dlog("Initializing threads...");
    for(int i = 0; i < tp->thread_count; i++) {
        struct wthread_arg *wtarg = malloc(sizeof(struct wthread_arg));
        wtarg->tp = tp;
        wtarg->id = i;
        dlogf("%s %d\n", "Initializing thread", i);
        thread_create(&threads[i], &worker_thread, wtarg);
    }
    thread_unlock_mutex(&tp->tp_lock);
}

void threadpool_destroy(ThreadPool *tp, enum shutdown_type type) {
    //set the shutting flag for the threads (we need to synchronize)
    thread_lock_mutex(&tp->tp_lock);
    //we are already shutting
    if(tp->shutting) {
        thread_unlock_mutex(&tp->tp_lock);
        return;
    }
    tp->shutting = type;
    thread_cond_signal_all(&tp->tasks_cond); //wake waiting threads
    thread_unlock_mutex(&tp->tp_lock);

    //wait for all the threads to finish
    dlog("Waiting for the threads to shut down...");
    for(int i = 0; i < tp->thread_count; i++)
        thread_join(&tp->threads[i]);

    //free all the resources
    thread_destroy_cond(&tp->tasks_cond);
    thread_destroy_mutex(&tp->tp_lock);
    free(tp->threads);
    free(tp);
}

int threadpool_add_task(ThreadPool *tp, void (*task_func)(void *, int), void *args) {
    ThreadPoolTask *task = malloc(sizeof(ThreadPoolTask));
    if(!task) return -1;
    task->next = NULL;
    task->task_func = task_func;
    task->args = args;

    thread_lock_mutex(&tp->tp_lock);
    dlog("Adding task to the thread pool queue...");
    if(tp->queue_size == 0) {
        tp->tasks_head = task;
    } else {
        tp->tasks_tail->next = task;
    }
    tp->tasks_tail = task;
    tp->queue_size++;
    thread_cond_signal_all(&tp->tasks_cond);
    thread_unlock_mutex(&tp->tp_lock);
    return 0;
}

static void worker_thread(void *worker_thread_arg) {
    struct wthread_arg *wtarg = worker_thread_arg;

    ThreadPool *tp = wtarg->tp;
    int id = wtarg->id;

    free(wtarg);

    for(;;) {
        thread_lock_mutex(&tp->tp_lock);
        //if the queue is empty wait on cv
        while((tp->queue_size == 0) && !(tp->shutting)) {
            dlogf("%s %d %s\n", "Thread", id, "waiting for tasks");
            thread_cond_wait(&tp->tasks_cond, &tp->tp_lock);
        }

        if(tp->shutting == HARD_SHUTDOWN || (tp->shutting == SOFT_SHUTDOWN && tp->queue_size == 0)) {
            dlogf("%s %d %s\n", "Thread", id, "shutting...");
            thread_unlock_mutex(&tp->tp_lock);
            break;
        }

        dlogf("%s %d %s\n", "Thread", id, "obtaining task");
        ThreadPoolTask *task = tp->tasks_head;
        tp->tasks_head = task->next;
        tp->queue_size--;
        thread_unlock_mutex(&tp->tp_lock);

        void (*task_func)(void *, int) = task->task_func;
        void *args = task->args;
        free(task); //free the task struct

        dlogf("%s %d %s\n", "Thread", id, "executing task...");
        (*task_func)(args, id);
        dlogf("%s %d %s\n", "Thread", id, "done");
    }
}
