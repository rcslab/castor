
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/capsicum.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "system.h"
#include "util.h"

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
thread_local uint32_t threadId = 0; //-1;

extern int
_pthread_create(pthread_t * thread, const pthread_attr_t * attr,
	       void *(*start_routine) (void *), void *arg);

/*
 * Environment Variables:
 * CASTOR_MODE		RECORD, REPLAY
 * CASTOR_LOGFILE	Record/Replay File Path
 * CASTOR_HOST		Fault-Tolerance Master Hostname
 */

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

    while (1) {
	uint64_t numEntries;
	RRLogEntry *entry = RRGlobalQueue_Dequeue(&rrgq, &numEntries);

	if (numEntries) {
	    SystemWrite(logfd, entry, numEntries * sizeof(RRLogEntry));
	}

	for (uint64_t i = 0; i < numEntries; i++) {
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
    if (primeSawExit)
	pthread_exit(NULL);

    while (1) {
	uint64_t numEntries;
	RRLogEntry *entry;

	do {
	    entry = RRGlobalQueue_Dequeue(&rrgq, &numEntries);
	} while (numEntries == 0);

	for (uint64_t i = 0; i < numEntries; i++) {
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
    while (1) {
	uint64_t numEntries = 512;
	RRLogEntry entries[512];

	int result = SystemRead(logfd, &entries, numEntries * sizeof(RRLogEntry));
	if (result < 0) {
	    perror("read");
	    abort();
	}
	if (result == 0) {
	    return NULL;
	}

	numEntries = (uint64_t)result / sizeof(RRLogEntry);

	for (uint64_t i = 0; i < numEntries; i++) {
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
    uint64_t numEntries = 32;
    RRLogEntry entries[32];

    int result = SystemRead(logfd, &entries, numEntries * sizeof(RRLogEntry));
    if (result < 0) {
	perror("read");
	abort();
    }

    numEntries = (uint64_t)result / sizeof(RRLogEntry);

    for (uint64_t i = 0; i < numEntries; i++) {
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
	ftMode = true;
	rrMode = RRMODE_RECORD;
	abort();
    } else if (strcmp(mode, "SLAVE") == 0) {
	fprintf(stderr, "SLAVE\n");
	ftMode = true;
	rrMode = RRMODE_REPLAY;
	abort();
    } else {
	fprintf(stderr, "Error: Unknown CASTOR_MODE\n");
    }

    rrlog = malloc(RRLOG_DEFAULT_REGIONSZ);
    if (!rrlog){
	fprintf(stderr, "Could not allocate record/replay log\n");
	abort();
    }

    RRLog_Init(rrlog, (PAGESIZE - 4*CACHELINE)/CACHELINE);
    threadId = RRShared_AllocThread(rrlog);
    RRGlobalQueue_Init(&rrgq);
    Events_Init();

    int flags = O_RDWR;

    if (logpath == NULL) {
	fprintf(stderr, "Error: Must specify log path CASTOR_LOGFILE\n");
	abort();
    }

    if (rrMode == RRMODE_RECORD)
	flags |= O_CREAT | O_TRUNC;

    logfd = open(logpath, flags, 0600);
    if (logfd < 0) {
	perror("open");
	abort();
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

    char *shmpath = getenv("CASTOR_SHMPATH");
    if (shmpath == NULL) {
	OldInit();
	return;
    }

    char *mode = getenv("CASTOR_MODE");
    if (mode == NULL) {
	rrMode = RRMODE_NORMAL;
	return;
    }

    Debug_Init("castor.log");

    // Make sure LogDone returns immediately
    atomic_store(&drainDone, 1);

    key_t shmkey = ftok(shmpath, 0);
    if (shmkey == -1) {
	perror("ftok");
	abort();
    }

    int shmid = shmget(shmkey, RRLOG_DEFAULT_REGIONSZ, 0);
    if (shmid == -1) {
	perror("shmget");
	abort();
    }

    rrlog = shmat(shmid, NULL, 0);
    if (rrlog == MAP_FAILED) {
	perror("shmat");
	abort();
    }

    RRShared_SetupThread(rrlog, 0);
    threadId = 0;

    // XXX: Need to remap the region again to the right size
    assert(RRLOG_DEFAULT_REGIONSZ == rrlog->regionSz);

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
