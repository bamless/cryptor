#include "threadpool.h"
#include "thread.h"
#include "logging.h"

#include <stdlib.h>

/*A threadpool task*/
typedef struct ThreadPoolTask {
    struct ThreadPoolTask *next;    /*Pointer to next task*/
    void (*task_func)(void *, int); /*The function of the task, to be executed from a worker thread*/
    void *args;                     /*The args to be passed to the function*/
} ThreadPoolTask;

/*The actual threadpool struct*/
struct ThreadPool {
    int thread_count;               /*The number of worker threads*/
    int queue_size;                 /*The number of tasks currently in the queue*/
    ThreadPoolTask *tasks_head;     /*The queue head*/
    ThreadPoolTask *tasks_tail;     /*The queue tail*/
    int shutting;                   /*Flag indicating if the threadpool is shutting*/
    Thread *threads;                /*Array of size thread_count containing the worker threads*/
    Mutex tp_lock;                  /*Mutex for synchronizing access*/
    CondVar tasks_cond;             /*Condition var for signaling the thrads when a task is inserted*/
};

/*struct that will be passed to the working thread on creation*/
struct wthread_arg {
    ThreadPool *tp;                 /*The threadpool of the working thread*/
    int id;                         /*The id of the working thread*/
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
    for(int i = 0; i < tp->thread_count; i++) {
        struct wthread_arg *wtarg = malloc(sizeof(struct wthread_arg));
        wtarg->tp = tp;
        wtarg->id = i;
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
    for(int i = 0; i < tp->thread_count; i++) {
        thread_join(&tp->threads[i]);
    }

    //free all the resources
    thread_destroy_cond(&tp->tasks_cond);
    thread_destroy_mutex(&tp->tp_lock);
    //if the shutdown type is hard we need to free the remaining tasks
    if(type == HARD_SHUTDOWN) {
        ThreadPoolTask *t;
        while((t = tp->tasks_head)) {
            tp->tasks_head = t->next;
            free(t);
        }
    }
    free(tp->threads);
    free(tp);
}

int threadpool_add_task(ThreadPool *tp, void (*task_func)(void *, int), void *args) {
    ThreadPoolTask *task = malloc(sizeof(ThreadPoolTask));
    if(!task) return ERR_OUTOFMEM;
    task->next = NULL;
    task->task_func = task_func;
    task->args = args;

    thread_lock_mutex(&tp->tp_lock);

    //if shutting return error
    if(tp->shutting) {
        free(task);
        thread_unlock_mutex(&tp->tp_lock);
        return ERR_SHUTDOWN;
    }

    //add the task to the queue
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
            thread_cond_wait(&tp->tasks_cond, &tp->tp_lock);
        }

        if(tp->shutting == HARD_SHUTDOWN || (tp->shutting == SOFT_SHUTDOWN && tp->queue_size == 0)) {
            thread_unlock_mutex(&tp->tp_lock);
            break;
        }

        //obtain task from queue
        ThreadPoolTask *task = tp->tasks_head;
        tp->tasks_head = task->next;
        if(!tp->tasks_head) tp->tasks_tail = NULL;
        tp->queue_size--;

        thread_unlock_mutex(&tp->tp_lock);

        void (*task_func)(void *, int) = task->task_func;
        void *args = task->args;
        free(task); //free the task struct

        (*task_func)(args, id);
    }
}
