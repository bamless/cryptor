#include "thread.h"
#include "logging.h"

#include <pthread.h>
#include <stdlib.h>

struct Thread {
  pthread_t tid;
};

/**Windows and Linux take in a start function with different return type. In order
to create a unified interface this library takes in a start func with a void return
type and then encapsulates it in a platform-specific function with the platform's
return type. The funcargs_t struct takes in the pointer to our platform agnostic
function and void* args and gets passed as an input to the plaform-specific start func.*/
struct funcargs_t {
    void (*func)(void *);
    void *args;
};
static void* start_func_impl(void *func_args);

Thread* thread_create(void (*func)(void *), void *arg) {
    pthread_t id;

    struct funcargs_t *fa = malloc(sizeof(struct funcargs_t));
    fa->func = func;
    fa->args = arg;

    int err = pthread_create(&id, NULL, &start_func_impl, fa);
    if(err) return NULL;

    Thread *thread = malloc(sizeof(Thread));
    if(!thread) {
        elog("Error: thread_create: out of memory");
        return NULL;
    }
    thread->tid = id;
    return thread;
}

void thread_free(Thread *thread) {
    free(thread);
}

int thread_join(Thread *thread) {
    return pthread_join(thread->tid, NULL);
}

static void* start_func_impl(void *func_args) {
    struct funcargs_t *fa = func_args;
    void (*func)(void *) = fa->func;
    void *args = fa->args;
    free(fa);
    (*func)(args);
    return NULL;
}

struct Mutex {
    pthread_mutex_t mutex;
};

Mutex* thread_create_mutex() {
    Mutex *m = malloc(sizeof(Mutex));
    if(!m) {
        elog("Error: thread_create_mutex: out of memory");
        return NULL;
    }

    int err = 0;
    pthread_mutexattr_t attr;

    err |= pthread_mutexattr_init(&attr);
    err |= pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    err |= pthread_mutex_init(&m->mutex, &attr);

    if(err) {
        free(m);
        return NULL;
    }
    return m;
}

int thread_destroy_mutex(Mutex *mutex) {
    int err = pthread_mutex_destroy(&mutex->mutex);
    free(mutex);
    return err; //TODO: create specific errors
}

int thread_lock_mutex(Mutex *mutex) {
    return pthread_mutex_lock(&mutex->mutex);
}

int thread_unlock_mutex(Mutex *mutex) {
    return pthread_mutex_unlock(&mutex->mutex);
}
