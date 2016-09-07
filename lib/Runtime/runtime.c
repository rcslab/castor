
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/capsicum.h>

#include <sys/syscall.h>

#include <castor/rrevent.h>
#include "rrlog.h"
#include "rrplay.h"
#include <castor/rrgq.h>
#include <castor/runtime.h>

extern void Events_Init();

static uint64_t logOffset;
RRLog *rrlog;
static alignas(PAGESIZE) RRGlobalQueue rrgq;
static int logfd;
pthread_t rrthr;
pthread_t gqthr;

static bool volatile primeSawExit = false;
atomic_bool drainDone = false;
bool ftMode = false;
enum RRMODE rrMode = RRMODE_NORMAL;
thread_local int threadId = 0; //-1;

extern int
_pthread_create(pthread_t * thread, const pthread_attr_t * attr,
	       void *(*start_routine) (void *), void *arg);

/*
 * Environment Variables:
 * CASTOR_MODE		RECORD, REPLAY
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
    logOffset = 0;

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

    SystemWrite(1, "Consumer Done\n", 14);
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
	    SystemWrite(logfd, entry, numEntries * sizeof(RRLogEntry));
	}

	for (i = 0; i < numEntries; i++) {
	    if (entry[i].event == RREVENT_EXIT) {
		SystemWrite(1, "TXGQ Done\n", 10);
		fsync(logfd);
		atomic_store(&drainDone, true);
		return NULL;
	    }
	}

	RRGlobalQueue_Free(&rrgq, numEntries);

	//usleep(10000);
	pthread_yield();
    }
}

void *
FeedQueue(void *arg)
{
    uint64_t i;
    uint64_t numEntries;
    RRLogEntry *entry;

    if (primeSawExit)
	pthread_exit(NULL);

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

	int result = SystemRead(logfd, &entries, numEntries * sizeof(RRLogEntry));
	if (result < 0) {
	    perror("read");
	    abort();
	}
	if (result == 0) {
	    return NULL;
	}

	numEntries = result / sizeof(RRLogEntry);

	for (i = 0; i < numEntries; i++) {
	    RRGlobalQueue_Append(&rrgq, &entries[i]);
	    if (entries[i].event == RREVENT_EXIT) {
		//printf("RXGQ Done\n");
		atomic_store(&drainDone, true);
		return NULL;
	    }
	}
    }
}

bool
PrimeQ()
{
    int numEntries = 32;
    RRLogEntry entries[32];

    int result = SystemRead(logfd, &entries, numEntries * sizeof(RRLogEntry));
    if (result < 0) {
	perror("read");
	abort();
    }

    numEntries = result / sizeof(RRLogEntry);

    for (int i = 0; i < numEntries; i++) {
	RRPlay_AppendThread(rrlog, &entries[i]);
	if (entries[i].event == RREVENT_EXIT) {
	    return true;
	}
    }

    return false;
}

void
LogDone()
{
    /*pthread_join(rrthr, NULL);
    pthread_join(gqthr, NULL);*/
    while (!atomic_load(&drainDone)) {
	usleep(0);
    }
}

void
OldInit()
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
	abort();
	ftMode = true;
	rrMode = RRMODE_RECORD;
    } else if (strcmp(mode, "SLAVE") == 0) {
	fprintf(stderr, "SLAVE\n");
	abort();
	ftMode = true;
	rrMode = RRMODE_REPLAY;
    } else {
	fprintf(stderr, "Error: Unknown CASTOR_MODE\n");
    }

    rrlog = malloc(sizeof(*rrlog));
    if (!rrlog){
	fprintf(stderr, "Could not allocate record/replay log\n");
	abort();
	return;
    }

    RRLog_Init(rrlog);
    RRGlobalQueue_Init(&rrgq);
    Events_Init();

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

    if (rrMode == RRMODE_RECORD) {
	_pthread_create(&rrthr, NULL, DrainQueue, NULL);
	_pthread_create(&gqthr, NULL, TXGQProc, NULL);
    } else if (rrMode == RRMODE_REPLAY) {
	primeSawExit = PrimeQ();
	_pthread_create(&rrthr, NULL, FeedQueue, NULL);
	_pthread_create(&gqthr, NULL, RXGQProc, NULL);
    }
}

__attribute__((constructor)) void
log_init()
{
    int status;
    struct stat sb;

    status = fstat(/* shmfd */3, &sb);
    if (status < 0) {
	//perror("fstat");
	OldInit();
	return;
    }

    char *mode = getenv("CASTOR_MODE");
    if (mode == NULL) {
	rrMode = RRMODE_NORMAL;
	return;
    }

    // Make sure LogDone returns immediately
    atomic_store(&drainDone, 1);

    rrlog = mmap(NULL, sizeof(*rrlog), PROT_READ|PROT_WRITE,
		MAP_NOSYNC|MAP_SHARED, /* shmfd */3, 0);
    if (rrlog == NULL) {
	perror("mmap");
	abort();
    }

    char *sandbox = getenv("CASTOR_SANDBOX");
    if (sandbox) {
	status = cap_enter();
	if (status != 0) {
	    perror("cap_enter");
	    abort();
	}
    }

    if (strcmp(mode, "RECORD") == 0) {
	fprintf(stderr, "RECORD\n");
	rrMode = RRMODE_RECORD;
    } else if (strcmp(mode, "REPLAY") == 0) {
	fprintf(stderr, "REPLAY\n");
	rrMode = RRMODE_REPLAY;
    }

    Events_Init();
}
