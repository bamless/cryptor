#ifndef THREAD_H
#define THREAD_H

typedef struct Thread Thread; // opaque type

Thread* thread_create(void (*func)(void *), void *arg);
void thread_free(Thread *thread);
int thread_join(Thread *thread);

// Synchronization

typedef struct Mutex Mutex; //opaque type

Mutex* thread_create_mutex();
int thread_destroy_mutex(Mutex *mutex);
int thread_lock_mutex(Mutex *mutex);
int thread_unlock_mutex(Mutex *mutex);

#endif
