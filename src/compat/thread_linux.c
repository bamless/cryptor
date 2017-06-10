#include "thread.h"
#include "logging.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct Thread {
  pthread_t tid;
};

static void error_check(const char *msg, int err);
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
    error_check("Error: thread_create", err);

    Thread *thread = malloc(sizeof(Thread));
    if(!thread) error_check("Error: thread_create", ENOMEM);

    thread->tid = id;
    return thread;
}

void thread_free(Thread *thread) {
    free(thread);
}

void thread_join(Thread *thread) {
    int err = pthread_join(thread->tid, NULL);
    error_check("Error: thread_join", err);
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
    if(!m) error_check("Error: thread_create_mutex", ENOMEM);

    int err = 0;
    pthread_mutexattr_t attr;

    err = pthread_mutexattr_init(&attr);
    error_check("Error: thread_create_mutex", err);
    err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    error_check("Error: thread_create_mutex", err);
    err = pthread_mutex_init(&m->mutex, &attr);
    error_check("Error: thread_create_mutex", err);

    return m;
}

void thread_destroy_mutex(Mutex *mutex) {
    int err = pthread_mutex_destroy(&mutex->mutex);
    free(mutex);
    error_check("Error: thread_destroy_mutex", err);
}

void thread_lock_mutex(Mutex *mutex) {
    int err =  pthread_mutex_lock(&mutex->mutex);
    error_check("Error: thread_lock_mutex", err);
}

void thread_unlock_mutex(Mutex *mutex) {
    int err = pthread_mutex_unlock(&mutex->mutex);
    error_check("Error: thread_unlock_mutex", err);
}

static void error_check(const char *msg, int err) {
    if(err) {
        char buff[256];
        strerror_r(err, buff, 256);
        elogf("%s: %s\n", msg, buff);
        exit(err);
    }
}
