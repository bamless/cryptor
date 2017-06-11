#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "files.h"
#include "logging.h"
#include "fileUtils.h"
#include "thread.h"
#include "utilsCompat.h"

#define NUM_THREADS 6

static CondVar cv;
static Mutex m;
static int jobs;

static void start_func(void *args) {
	thread_lock_mutex(&m);
	while(jobs == 0)
		thread_cond_wait(&cv, &m);

	jobs--;
	SLEEP(500);
	logs((char *) args);
	thread_unlock_mutex(&m);
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
		thread_create(&threads[i], &start_func, names[i]);
	}

	SLEEP(5000);
	thread_lock_mutex(&m);
	jobs++;
	thread_cond_signal_all(&cv);
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
