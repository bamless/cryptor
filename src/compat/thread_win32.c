//Windows Vista code (I don't know why but even if compiling on win10 this is set to an old O.S value)
#define _WIN32_WINNT 0x0600
#include "thread.h"
#include "logging.h"
#include "error.h"

#include <windows.h>
#include <synchapi.h>

struct funcargs {
    void (*func)(void *);
    void *args;
};
static DWORD WINAPI start_func_impl(void *args);

void thread_create(Thread *thread, void (*func)(void *), void *arg) {
    struct funcargs *fa = malloc(sizeof(struct funcargs));
    fa->func = func;
    fa->args = arg;

    DWORD tid;
    *thread = CreateThread(NULL, 0, &start_func_impl, fa, 0, &tid);
    if(*thread == INVALID_HANDLE_VALUE) {
        perr("Error: thread_create");
        exit(1);
    }
}


static DWORD WINAPI start_func_impl(void *func_args) {
    struct funcargs *fa = (struct funcargs *) func_args;
    void (*func)(void *) = fa->func;
    void *args = fa->args;
    free(fa);
    (*func)(args);
    return 0;
}

void thread_join(Thread *thread) {
    if(WaitForSingleObject(*thread, INFINITE) == WAIT_FAILED) {
        perr("Error: thread_join");
        exit(1);
    }
}

// Synchronization
void thread_init_mutex(Mutex *mutex) {
    InitializeSRWLock(mutex);
}

void thread_destroy_mutex(Mutex *mutex) {
    //SRW Locks don't need to be destroyed
}

void thread_lock_mutex(Mutex *mutex) {
    AcquireSRWLockExclusive(mutex);
}

void thread_unlock_mutex(Mutex *mutex) {
    ReleaseSRWLockExclusive(mutex);
}

void thread_init_cond(CondVar *cond) {
    InitializeConditionVariable(cond);
}

void thread_destroy_cond(CondVar *cond) {
    //CV don't need to be freed in windows
}

void thread_cond_wait(CondVar *cond, Mutex *mutex) {
    SleepConditionVariableSRW(cond, mutex, INFINITE, 0);
}

void thread_cond_signal_all(CondVar *cond) {
    WakeAllConditionVariable(cond);
}
