
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>

#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include <sys/syscall.h>

#include "rrevent.h"
#include "rrlog.h"
#include "rrplay.h"
#include "rrgq.h"
#include "runtime.h"
#include "ft.h"

#define GLOBALLOG_LENGTH	(4*1024*1024)

static RRLogEntry *globalLog;
static uint64_t logOffset;
alignas(PAGESIZE) RRLog rrlog;
static alignas(PAGESIZE) RRGlobalQueue rrgq;
static int logfd;
pthread_t rrthr;
pthread_t gqthr;

bool ftMode = false;
enum RRMODE rrMode = RRMODE_NORMAL;
static volatile int nextThreadId = 1;
thread_local int threadId = 0; //-1;

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
    logOffset = 0;

    while (1) {
	RRLogEntry *entry = NULL;

	do {
	    entry = RRLog_Dequeue(&rrlog);
	} while (entry == NULL);

	RRGlobalQueue_Append(&rrgq, entry);

	if (entry->event == RREVENT_EXIT) {
	    RRLog_Free(&rrlog, entry);
	    break;
	}

	RRLog_Free(&rrlog, entry);
    }

    printf("Consumer Done\n");
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

	if (ftMode) {
	    RRFT_Send(numEntries, entry);
	} else {
	    SystemWrite(logfd, entry, numEntries * sizeof(RRLogEntry));
	}

	for (i = 0; i < numEntries; i++) {
	    if (entry[i].event == RREVENT_EXIT) {
		printf("TXGQ Done\n");
		return NULL;
	    }
	}

	RRGlobalQueue_Free(&rrgq, numEntries);

	sleep(0);
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
	    RRPlay_AppendThread(&rrlog, &entry[i]);
	    if (entry[i].event == RREVENT_EXIT) {
		printf("Feed Done\n");
		return NULL;
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

	    numEntries = result / sizeof(RRLogEntry);
	}

	for (i = 0; i < numEntries; i++) {
	    RRGlobalQueue_Append(&rrgq, &entries[i]);
	    if (entries[i].event == RREVENT_EXIT) {
		printf("RXGQ Done\n");
		return NULL;
	    }
	}
    }
}

void
LogDone()
{
    RRLogEntry *e;

    if (rrMode == RRMODE_RECORD) {
	e = RRLog_Alloc(&rrlog, threadId);
	e->event = RREVENT_EXIT;
	e->threadId = threadId;
	RRLog_Append(&rrlog, e);
    } else {
	e = RRPlay_Dequeue(&rrlog, threadId);
	assert(e->event == RREVENT_EXIT);
	RRPlay_Free(&rrlog, e);
    }

    pthread_join(rrthr, NULL);
    pthread_join(gqthr, NULL);
}

__attribute__((constructor)) void
log_init()
{
    char *mode = getenv("CASTOR_MODE");
    char *logpath = getenv("CASTOR_LOGFILE");

    if (mode == NULL) {
	rrMode = RRMODE_NORMAL;
	return;
    }

    if (strcmp(mode, "RECORD") == 0) {
	fprintf(stderr, "RECORD\n");
	rrMode = RRMODE_RECORD;
    } else if (strcmp(mode, "REPLAY") == 0) {
	fprintf(stderr, "REPLAY\n");
	rrMode = RRMODE_REPLAY;
    } else if (strcmp(mode, "MASTER") == 0) {
	fprintf(stderr, "MASTER\n");
	ftMode = true;
	rrMode = RRMODE_RECORD;
    } else if (strcmp(mode, "SLAVE") == 0) {
	fprintf(stderr, "SLAVE\n");
	ftMode = true;
	rrMode = RRMODE_REPLAY;
    } else {
	fprintf(stderr, "Error: Unknown CASTOR_MODE\n");
    }

    globalLog = malloc(GLOBALLOG_LENGTH);
    if (!globalLog) {
	fprintf(stderr, "Could not allocate global log\n");
	abort();
	return;
    }

    // Ensure memory is mapped
    memset((void *)globalLog, 0, GLOBALLOG_LENGTH);

    RRLog_Init(&rrlog);
    RRGlobalQueue_Init(&rrgq);
    Events_Init();

    if (ftMode) {
	if (rrMode == RRMODE_RECORD) {
	    RRFT_InitMaster();
	} else {
	    RRFT_InitSlave();
	}
    } else {
	int flags = O_RDWR;

	if (logpath == NULL) {
	    fprintf(stderr, "Error: Must specify log path CASTOR_LOGFILE\n");
	    abort();
	    return;
	}

	if (rrMode == RRMODE_RECORD)
	    flags |= O_CREAT | O_TRUNC;

	logfd = open(logpath, flags, 0600);
	if (logfd < 0) {
	    perror("open");
	    abort();
	    return;
	}
    }

    if (rrMode == RRMODE_RECORD) {
	atexit(LogDone);

	pthread_create(&rrthr, NULL, DrainQueue, NULL);
	pthread_create(&gqthr, NULL, TXGQProc, NULL);
    } else if (rrMode == RRMODE_REPLAY) {
	pthread_create(&rrthr, NULL, FeedQueue, NULL);
	pthread_create(&gqthr, NULL, RXGQProc, NULL);
    }
}

