#ifndef THREAD_H
#define THREAD_H

typedef struct Thread Thread; // opaque type

Thread* thread_create(void (*func)(void *), void *arg);
void thread_free(Thread *thread);
void thread_join(Thread *thread);

// Synchronization

typedef struct Mutex Mutex; //opaque type

Mutex* thread_create_mutex();
void thread_destroy_mutex(Mutex *mutex);
void thread_lock_mutex(Mutex *mutex);
void thread_unlock_mutex(Mutex *mutex);

typedef struct CondVar CondVar;

CondVar* thread_create_cond();
void thread_destroy_cond(CondVar *cond);
void thread_cond_wait(CondVar *cond, Mutex *mutex);
void thread_cond_signal_all(CondVar *cond);

#endif
