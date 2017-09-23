#ifndef THREADPOOL_H
#define THREADPOOL_H

typedef struct ThreadPool ThreadPool; //opaque, to provide encapsulation

enum ShutDownType {
	/*The worker threads empty the queue before shutting*/
	SOFT_SHUTDOWN = 1,
	/*The worker threads shut down as soon as they can*/
	HARD_SHUTDOWN = 2
};

#define ERR_SHUTDOWN 1
#define ERR_OUTOFMEM 2

/*
 * Creates a new thread pool and returns a pointer to it
 * @arg thread_count The number of working threads of this threadpool
 * @return the ThreadPool on success, NULL if an error occurred
 */
ThreadPool* threadpool_create(int thread_count);
/*
 * Destroys the threadpool. It signals all the threads to stop, joins on them
 * and then frees all the resource associated with the threadpool.
 * @arg type The type of shutdown the thread pool will perform. @see shutdown_type
 */
void threadpool_destroy(ThreadPool *tp, enum ShutDownType type);
/*
 * Adds a new task to the thread pool
 * @arg task_func The pointer to the function that will be executed
 * @arg args The argument that will be passed as input to task_func when executed
 * @return ERR_SHUTDOWN if the threadpool is shutting down, ERR_OUTOFMEM if failed to allocate,
 *		   0 on success
 */
int threadpool_add_task(ThreadPool *tp, void (*task_func)(void *), void *args);

#endif
