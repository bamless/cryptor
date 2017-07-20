#include "thread.h"
#include "logging.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct funcargs {
    void (*func)(void *, void *);
    void *args;
    void *retval;
};
//wrapper for pthread start function.
static void* start_func_impl(void *funcargs);
//pthread functions are not guaranteed to set errno, so we must obtain the error msg from the error returned
static void error_check(const char *msg, int err);

void thread_create(Thread *thread, void (*func)(void *, void *), void *arg, void *retval) {
    pthread_t id;

    struct funcargs *fa = malloc(sizeof(struct funcargs));
    if(!fa) error_check("Error: thread_create", ENOMEM);
    fa->func = func;
    fa->args = arg;
    fa->retval = retval;

    int err = pthread_create(&id, NULL, &start_func_impl, fa);
    error_check("Error: thread_create", err);

    *thread = id;
}

void thread_join(Thread *thread) {
    int err = pthread_join(*thread, NULL);
    error_check("Error: thread_join", err);
}

static void* start_func_impl(void *funcargs) {
    struct funcargs *fa = (struct funcargs *) funcargs;
    void (*func)(void *, void *) = fa->func;
    void *args = fa->args;
    void *retval = fa->retval;
    free(fa);
    (*func)(args, retval);
    return NULL;
}

//Synchronization
void thread_init_mutex(Mutex *mutex) {
    int err = 0;
    pthread_mutexattr_t attr;

    err = pthread_mutexattr_init(&attr);
    error_check("Error: thread_init_mutex", err);
    err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    error_check("Error: thread_init_mutex", err);
    err = pthread_mutex_init(mutex, &attr);
    error_check("Error: thread_init_mutex", err);
}

void thread_destroy_mutex(Mutex *mutex) {
    int err = pthread_mutex_destroy(mutex);
    error_check("Error: thread_destroy_mutex", err);
}

void thread_lock_mutex(Mutex *mutex) {
    int err =  pthread_mutex_lock(mutex);
    error_check("Error: thread_lock_mutex", err);
}

void thread_unlock_mutex(Mutex *mutex) {
    int err = pthread_mutex_unlock(mutex);
    error_check("Error: thread_unlock_mutex", err);
}

void thread_init_cond(CondVar *cond) {
    int err = pthread_cond_init(cond, NULL);
    error_check("Error: thread_init_cond", err);
}

void thread_destroy_cond(CondVar *cond) {
    int err = pthread_cond_destroy(cond);
    error_check("Error: pthread_destroy_cond", err);
}

void thread_cond_wait(CondVar *cond, Mutex *mutex) {
    int err = pthread_cond_wait(cond, mutex);
    error_check("Error: thread_cond_wait", err);
}

void thread_cond_signal_all(CondVar *cond) {
    int err = pthread_cond_broadcast(cond);
    error_check("Error: thread_cond_signal_all", err);
}

void thread_cond_signal(CondVar *cond) {
    int err = pthread_cond_signal(cond);
    error_check("Error: thread_cond_signal", err);
}

static void error_check(const char *msg, int err) {
    if(err) {
        char buff[256];
        strerror_r(err, buff, 256);
        elogf("%s: %s\n", msg, buff);
        exit(EXIT_FAILURE);
    }
}
