#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "files.h"
#include "logging.h"
#include "fileUtils.h"
#include "thread.h"
#include "utilsCompat.h"

#define NUM_THREADS 20

static Mutex *m;

static void start_func(void *args) {
	thread_lock_mutex(m);
	SLEEP(1000);
	logs((char *) args);
	//recursive_list("folder");
	thread_unlock_mutex(m);
}

int main() {
	m = thread_create_mutex();

	Thread *threads[NUM_THREADS];
	char *names[NUM_THREADS];
	for(int i = 0; i < NUM_THREADS; i++) {
		names[i] = malloc(sizeof(char) * 12);
		snprintf(names[i], 11, "Thread %d", i);
		threads[i] = thread_create(&start_func, names[i]);
	}

	logs("before");
	for(int i = 0; i < NUM_THREADS; i++) {
		thread_join(threads[i]);
	}
	logs("after");

	for(int i = 0; i < NUM_THREADS; i++) {
		free(names[i]);
		thread_free(threads[i]);
	}
	thread_destroy_mutex(m);
}
