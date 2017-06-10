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

#endif
