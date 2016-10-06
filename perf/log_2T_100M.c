
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <time.h>
#include <pthread.h>
#include <sys/mman.h>

#include <castor/rrlog.h>

RRLog *rrlog;

#define TEST_THREADS		2ULL
#define EVENTS_PER_THREAD	100000000ULL

void *
test_producer(void *arg)
{
    uint64_t i;
    uint64_t threadId = (uint64_t)arg;

    for (i = 0; i < EVENTS_PER_THREAD; i++) {
        RRLogEntry *e = RRLog_Alloc(rrlog, threadId);
	e->objectId = 1;
        RRLog_Append(rrlog, e);
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
	    entry = RRLog_Dequeue(rrlog);
	} while (entry == NULL);

	RRLog_Free(rrlog, entry);
    }
}

int
main(int argc, const char *argv[])
{
    uint64_t i;
    pthread_t tid;

    rrlog = mmap(NULL, RRLOG_DEFAULT_REGIONSZ, PROT_READ|PROT_WRITE,
		MAP_NOSYNC|MAP_ANONYMOUS, -1, 0);
    if (rrlog == MAP_FAILED) {
	perror("mmap");
	abort();
    }

    memset(rrlog, 0, RRLOG_DEFAULT_REGIONSZ);

    RRLog_Init(rrlog, (PAGESIZE - 4*CACHELINE)/CACHELINE);
    rrlog->regionSz = RRLOG_DEFAULT_REGIONSZ;

    for (i = 0; i < TEST_THREADS; i++) {
	RRShared_SetupThread(rrlog, i);
	pthread_create(&tid, NULL, test_producer, (void *)i);
    }

    test_consumer();
}

