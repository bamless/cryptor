#include "socket.h"
#include "threadpool.h"
#include "logging.h"
#include "thread.h"
#include "error.h"
#include "files.h"
#include "stringbuf.h"
#include "mmap.h"
#include "encrypt.h"

#include <omp.h>
#include <math.h>
#include <stdlib.h>
#include <inttypes.h>

#ifdef __unix
#include <unistd.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __unix
    #define SLEEP(x) usleep((x) * 1000);
#endif
#ifdef _WIN32
    #define SLEEP(x) Sleep((x));
#endif

#define NUM_THREADS 7

static CondVar cv;
static Mutex m;
static int jobs;

static void start_func(void *args, void **retval) {
	for(;;) {
		thread_lock_mutex(&m);
		while(jobs == 0)
			thread_cond_wait(&cv, &m);

		jobs--;
		SLEEP(500);
		logs((char *) args);
		thread_unlock_mutex(&m);
		SLEEP(20000);
	}
}

int main() {
	thread_init_cond(&cv);
	thread_init_mutex(&m);
	jobs = 5;

	Thread threads[NUM_THREADS];
	char *names[NUM_THREADS];
	for(int i = 0; i < NUM_THREADS; i++) {
		names[i] = malloc(sizeof(char) * 12);
		snprintf(names[i], 12, "Thread %d", i);
		thread_create(&threads[i], &start_func, names[i], NULL);
	}

	SLEEP(5000);
	thread_lock_mutex(&m);
	jobs += 4;
	for(int i = 0; i < 4; i++)
		thread_cond_signal(&cv);
	thread_unlock_mutex(&m);

	logs("before");
	for(int i = 0; i < NUM_THREADS; i++) {
		thread_join(&threads[i]);
	}
	logs("after");

	for(int i = 0; i < NUM_THREADS; i++) {
		free(names[i]);
	}
	thread_destroy_mutex(&m);
	thread_destroy_cond(&cv);
}
