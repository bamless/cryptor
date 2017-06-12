#ifndef THREAD_H
#define THREAD_H

//includes
#ifdef __unix
#include <pthread.h>
#endif
#ifdef _WIN32
#include <windows.h>
#include <synchapi.h>
#endif

//Threads

#ifdef __unix
typedef pthread_t Thread;
#endif
#ifdef _WIN32
typedef HANDLE Thread;
#endif

void thread_create(Thread *thread, void (*func)(void *), void *arg);
void thread_join(Thread *thread);

// Synchronization

#ifdef __unix
typedef pthread_mutex_t Mutex;
#endif
#ifdef _WIN32
typedef SRWLOCK Mutex;
#endif

void thread_init_mutex(Mutex *mutex);
void thread_destroy_mutex(Mutex *mutex);
void thread_lock_mutex(Mutex *mutex);
void thread_unlock_mutex(Mutex *mutex);

#ifdef __unix
typedef pthread_cond_t CondVar;
#endif
#ifdef _WIN32
typedef CONDITION_VARIABLE CondVar;
#endif

void thread_init_cond(CondVar *cond);
void thread_destroy_cond(CondVar *cond);
void thread_cond_wait(CondVar *cond, Mutex *mutex);
void thread_cond_signal_all(CondVar *cond);

#endif
