
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <time.h>
#include <pthread.h>

#include <castor/rrlog.h>

static RRLog rrlog;

#define TEST_THREADS		1ULL
#define EVENTS_PER_THREAD	100000000ULL

void *
test_producer(void *arg)
{
    uint64_t i;
    uint32_t threadId = (uint32_t)arg;

    for (i = 0; i < EVENTS_PER_THREAD; i++) {
        RRLogEntry *e = RRLog_Alloc(&rrlog, threadId);
	e->objectId = 1;
	e->event = 1;
        RRLog_Append(&rrlog, e);
    }

    return NULL;
}

void
test_consumer()
{
    uint64_t i;

    for (i = 0; i < TEST_THREADS * EVENTS_PER_THREAD; i++) {
	RRLogEntry *entry = NULL;

	do {
	    entry = RRLog_Dequeue(&rrlog);
	} while (entry == NULL);

	RRLog_Free(&rrlog, entry);
    }
}

int
main(int argc, const char *argv[])
{
    uint64_t i;
    pthread_t tid;

    RRLog_Init(&rrlog);

    for (i = 0; i < TEST_THREADS; i++) {
	pthread_create(&tid, NULL, test_producer, (void *)i);
    }

    test_consumer();
}

