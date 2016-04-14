
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>

#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>

#include <sys/syscall.h>

#include <rrevent.h>
#include <rrlog.h>
#include <rrplay.h>
#include <rrgq.h>

#include "runtime.h"
#include "ft.h"

static RRLog *rrlog;
static alignas(PAGESIZE) RRGlobalQueue rrgq;
static int logfd;
static int shmfd;
pthread_t rrthr;
pthread_t gqthr;

bool ftMode = false;

/*
 * Environment Variables:
 * CASTOR_MODE		RECORD, REPLAY, MASTER, SLAVE
 * CASTOR_LOGFILE	Record/Replay File Path
 * CASTOR_HOST		Fault-Tolerance Master Hostname
 */

int
SystemRead(int fd, void *buf, size_t nbytes)
{
    return syscall(SYS_read, fd, buf, nbytes);
}

int
SystemWrite(int fd, const void *buf, size_t nbytes)
{
    return syscall(SYS_write, fd, buf, nbytes);
}

void *
DrainQueue(void *arg)
{
    uint64_t thrDone = 0;

    while (1) {
	RRLogEntry *entry = NULL;

	do {
	    entry = RRLog_Dequeue(rrlog);
	} while (entry == NULL);

	RRGlobalQueue_Append(&rrgq, entry);

	if (entry->event == RREVENT_EXIT) {
	    RRLog_Free(rrlog, entry);
	    break;
	}

	RRLog_Free(rrlog, entry);
    }

    //printf("Consumer Done\n");
    return NULL;
}

void *
TXGQProc(void *arg)
{
    uint64_t i;
    RRLogEntry *entry;
    uint64_t numEntries;

    while (1) {
	entry = RRGlobalQueue_Dequeue(&rrgq, &numEntries);

	if (numEntries) {
	    if (ftMode) {
		RRFT_Send(numEntries, entry);
	    } else {
		SystemWrite(logfd, entry, numEntries * sizeof(RRLogEntry));
	    }
	}

	for (i = 0; i < numEntries; i++) {
	    if (entry[i].event == RREVENT_EXIT) {
		printf("TXGQ Done\n");
		return NULL;
	    }
	}

	RRGlobalQueue_Free(&rrgq, numEntries);

	usleep(100000);
    }
}

void *
FeedQueue(void *arg)
{
    uint64_t i;
    uint64_t numEntries;
    RRLogEntry *entry;

    while (1) {
	do {
	    entry = RRGlobalQueue_Dequeue(&rrgq, &numEntries);
	} while (numEntries == 0);

	for (i = 0; i < numEntries; i++) {
	    RRPlay_AppendThread(rrlog, &entry[i]);
	    if (entry[i].event == RREVENT_EXIT) {
		//printf("Feed Done\n");
		pthread_exit(NULL);
		__builtin_unreachable();
	    }
	}

	RRGlobalQueue_Free(&rrgq, numEntries);
    }
}

void *
RXGQProc(void *arg)
{
    uint64_t i;
    uint64_t numEntries;
    RRLogEntry entries[512];

    while (1) {
	numEntries = 512;

	if (ftMode) {
	    numEntries = RRFT_Recv(numEntries, (RRLogEntry *)&entries);
	} else {
	    int result = SystemRead(logfd, &entries, numEntries * sizeof(RRLogEntry));
	    if (result < 0) {
		perror("read");
		abort();
	    }
	    if (result == 0) {
		return NULL;
	    }

	    numEntries = result / sizeof(RRLogEntry);
	}

	for (i = 0; i < numEntries; i++) {
	    RRGlobalQueue_Append(&rrgq, &entries[i]);
	    if (entries[i].event == RREVENT_EXIT) {
		//printf("RXGQ Done\n");
		return NULL;
	    }
	}
    }
}

void
OpenLog(const char *logfile, bool forRecord)
{
    char buf[4096];

    // Setup shared memory region
    shmfd = open("rr.shm", O_RDWR|O_CREAT|O_TRUNC, 0755);
    if (shmfd < 0) {
	perror("open");
	abort();
    }

    memset(&buf, 0, 4096);
    for (int i = 0; i <= (sizeof(*rrlog)/4096); i++)
	write(shmfd, buf, 4096);

    assert(shmfd == 3);

    rrlog = mmap(NULL, sizeof(*rrlog), PROT_READ|PROT_WRITE,
		MAP_NOSYNC|MAP_SHARED, shmfd, 0);
    if (rrlog == NULL) {
	perror("mmap");
	abort();
    }

    // Open log file
    int flags = O_RDWR;
    if (forRecord) {
	flags |= O_CREAT|O_TRUNC;
    }

    logfd = open(logfile, flags, 0600);
    if (logfd < 0) {
	perror("open");
	abort();
	return;
    }

    RRLog_Init(rrlog);
    RRGlobalQueue_Init(&rrgq);
}

void
LogDone()
{
    pthread_join(rrthr, NULL);
    pthread_join(gqthr, NULL);
}

void
RecordLog()
{
    pthread_create(&rrthr, NULL, DrainQueue, NULL);
    pthread_create(&gqthr, NULL, TXGQProc, NULL);
    LogDone();
}

void
ReplayLog()
{
    pthread_create(&rrthr, NULL, FeedQueue, NULL);
    pthread_create(&gqthr, NULL, RXGQProc, NULL);
    LogDone();
}


