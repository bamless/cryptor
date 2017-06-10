//Windows Vista code (I don't know why but even if compiling on win10 this is set to an old O.S value)
#define _WIN32_WINNT 0x0600
#include "thread.h"
#include "logging.h"

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
        elog("Error: thread_create: out of memory");
        return NULL;
    }

    struct funcargs_t *fa = malloc(sizeof(struct funcargs_t));
    fa->func = func;
    fa->args = arg;

    thread->hThread = CreateThread(NULL, 0, &start_func_impl, fa, 0, &thread->threadId);
    if(thread->hThread == INVALID_HANDLE_VALUE) {
        free(thread);
        return NULL;
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

int thread_join(Thread *thread) {
    return WaitForSingleObject(thread->hThread, INFINITE) == WAIT_OBJECT_0 ? 0 : 1;
}

// Synchronization

struct Mutex {
    SRWLOCK srw_lock;
};

Mutex* thread_create_mutex() {
    Mutex *mutex = malloc(sizeof(Mutex));
    if(!mutex) {
        elog("Error: thread_create_mutex: out of memory");
        return NULL;
    }
    InitializeSRWLock(&mutex->srw_lock);
    return mutex;
}

int thread_destroy_mutex(Mutex *mutex) {
    free(mutex); //SRW Locks don't need to be destroyed
    return 0;
}

int thread_lock_mutex(Mutex *mutex) {
    AcquireSRWLockExclusive(&mutex->srw_lock);
    return 0;
}

int thread_unlock_mutex(Mutex *mutex) {
    ReleaseSRWLockExclusive(&mutex->srw_lock);
    return 0;
}
