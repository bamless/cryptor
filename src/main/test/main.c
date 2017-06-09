#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "files.h"
#include "logging.h"
#include "fileUtils.h"
#include "thread.h"
#include "utilsCompat.h"

static Mutex *m;

static void start_func(void *args) {
	thread_lock_mutex(m);
	SLEEP(2000);
	logs((const char *) args);
	thread_unlock_mutex(m);
}

int main() {
	m = thread_create_mutex();
	Thread *t1 = thread_create(&start_func, "Thread 1");
	Thread *t2 = thread_create(&start_func, "Thread 2");
	
	logs("before");
	thread_join(t1);
	thread_join(t2);
	logs("after");

	thread_free(t1);
	thread_free(t2);
	thread_destroy_mutex(m);
}
