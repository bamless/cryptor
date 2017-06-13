#include "threadpool.h"
#include "logging.h"
#include "utilsCompat.h"

#define THREADS 3

static void thread_func(void *args, int id) {
	logf("%s %d %s\n", "Thread", id, "is doing stuff...");
	SLEEP(1000);
}

int main() {
	ThreadPool *tp = threadpool_create(THREADS);
	for(int i = 0; i < 2; i++)
		threadpool_add_task(tp, &thread_func, NULL);

	SLEEP(4000);
	threadpool_add_task(tp, &thread_func, NULL);
	threadpool_add_task(tp, &thread_func, NULL);
	threadpool_add_task(tp, &thread_func, NULL);
	threadpool_destroy(tp, SOFT_SHUTDOWN);
}
