//Windows Vista code (I don't know why but even if compiling on win10 this is set to an old O.S value)
#define _WIN32_WINNT 0x0600
#include "thread.h"
#include "logging.h"
#include "error.h"

#include <windows.h>
#include <synchapi.h>

struct Thread {
    HANDLE hThread;
    DWORD threadId;
};

struct funcargs_t {
    void (*func)(void *);
    void *args;
};
static DWORD WINAPI start_func_impl(void *args);

Thread* thread_create(void (*func)(void *), void *arg) {
    Thread *thread = malloc(sizeof(Thread));
    if(!thread) {
        SetLastError(ERROR_OUTOFMEMORY);
        exit(1);
    }

    struct funcargs_t *fa = malloc(sizeof(struct funcargs_t));
    fa->func = func;
    fa->args = arg;

    thread->hThread = CreateThread(NULL, 0, &start_func_impl, fa, 0, &thread->threadId);
    if(thread->hThread == INVALID_HANDLE_VALUE) {
        perr("Error: thread_create");
        exit(1);
    }
    return thread;
}


static DWORD WINAPI start_func_impl(void *func_args) {
    struct funcargs_t *fa = func_args;
    void (*func)(void *) = fa->func;
    void *args = fa->args;
    free(fa);
    (*func)(args);
    return 0;
}

void thread_free(Thread *thread) {
    free(thread);
}

void thread_join(Thread *thread) {
    if(WaitForSingleObject(thread->hThread, INFINITE) == WAIT_FAILED)
        perr("Error: thread_join");
}

// Synchronization

struct Mutex {
    SRWLOCK srw_lock;
};

Mutex* thread_create_mutex() {
    Mutex *mutex = malloc(sizeof(Mutex));
    if(!mutex) {
        SetLastError(ERROR_OUTOFMEMORY);
        exit(1);
    }
    InitializeSRWLock(&mutex->srw_lock);
    return mutex;
}

void thread_destroy_mutex(Mutex *mutex) {
    free(mutex); //SRW Locks don't need to be destroyed
}

void thread_lock_mutex(Mutex *mutex) {
    AcquireSRWLockExclusive(&mutex->srw_lock);
}

void thread_unlock_mutex(Mutex *mutex) {
    ReleaseSRWLockExclusive(&mutex->srw_lock);
}
