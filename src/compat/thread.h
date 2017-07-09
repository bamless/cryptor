#ifndef THREAD_H
#define THREAD_H

//includes
#ifdef __unix
#include <pthread.h>
#elif _WIN32
#include <windows.h>
#include <synchapi.h>
#endif

/*
 * Cross platform thread module. It encapsulates platform-specific thread API
 * and exposes a unified interface. The error checking is handled by the functions.
 */

/**Threads**/

#ifdef __unix
typedef pthread_t Thread;
#elif _WIN32
typedef HANDLE Thread;
#endif

/*
 * Creates a thread that starts executing the function `func`
 * @arg func the thread starting pointer. It takes `arg` and `retval` as inputs and returns void.
 * @arg arg a generic pointer to a user allocated argument. It will be passed to `func`
 * @arg retval a pointer to a pointer used to return a value at the calling thread
 *      It will be passed to `func`.
 */
void thread_create(Thread *thread, void (*func)(void *, void **), void *arg, void **retval);
/**Waits for the thread to finish*/
void thread_join(Thread *thread);

/**Synchronization**/

#ifdef __unix
typedef pthread_mutex_t Mutex;
#elif _WIN32
typedef SRWLOCK Mutex;
#endif

void thread_init_mutex(Mutex *mutex);
void thread_destroy_mutex(Mutex *mutex);
void thread_lock_mutex(Mutex *mutex);
void thread_unlock_mutex(Mutex *mutex);

#ifdef __unix
typedef pthread_cond_t CondVar;
#elif _WIN32
typedef CONDITION_VARIABLE CondVar;
#endif

void thread_init_cond(CondVar *cond);
void thread_destroy_cond(CondVar *cond);
void thread_cond_wait(CondVar *cond, Mutex *mutex);
void thread_cond_signal_all(CondVar *cond);
void thread_cond_signal(CondVar *cond);

#endif
