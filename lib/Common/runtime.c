
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
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include <sys/syscall.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>

#include <castor/events.h>
#include <castor/Common/runtime.h>
#include "ft.h"

static RRLog *rrlog;
static alignas(PAGESIZE) RRGlobalQueue rrgq;
static int logfd;
static int shmid = -1;
pthread_t rrthr;
pthread_t gqthr;

bool ftMode = false;

struct {
    uint32_t	evtid;
    const char* str;
} rreventTable[] =
{
#define RREVENT(_a, _b) { RREVENT_##_a, #_a },
    RREVENT_TABLE
#undef RREVENT
    { 0, 0 }
};

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
#if !defined(CASTOR_CTR)
    uint64_t i = 0;
#endif

    while (1) {
	RRLogEntry *entry = NULL;

#if defined(CASTOR_CTR)
	do {
	    entry = RRLog_Dequeue(rrlog);
	} while (entry == NULL);
#elif defined(CASTOR_TSC) || defined(CASTOR_TSX)

#if defined(CASTOR_FT)
	do {
	    entry = RRLogThreadDequeue(&rrlog->threads[i % RRLOG_MAX_THREADS]);
	    i++;
	} while (entry == NULL);
#elif defined(CASTOR_DBG) || defined(CASTOR_SNAP)
	do {
	    entry = RRLog_Dequeue(rrlog);
	    if (entry)
		break;

	    pthread_yield();
	} while (1);
#else
#error "Unknown mode"
#endif

#else
#error "Unknown backend"
#endif

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
    while (1) {
	uint64_t numEntries;
	RRLogEntry *entry = RRGlobalQueue_Dequeue(&rrgq, &numEntries);

	if (numEntries) {
	    if (ftMode) {
		RRFT_Send(numEntries, entry);
	    } else {
		SystemWrite(logfd, entry, numEntries * sizeof(RRLogEntry));
	    }
	}

	for (uint64_t i = 0; i < numEntries; i++) {
	    if (entry[i].event == RREVENT_EXIT) {
		printf("TXGQ Done\n");
		return NULL;
	    }
	}

	RRGlobalQueue_Free(&rrgq, numEntries);

	//usleep(100000);
	pthread_yield();
    }
}

void *
FeedQueue(void *arg)
{
    while (1) {
	uint64_t numEntries;
	RRLogEntry *entry;

	do {
	    entry = RRGlobalQueue_Dequeue(&rrgq, &numEntries);
	} while (numEntries == 0);

	for (uint64_t i = 0; i < numEntries; i++) {
	    if (!RRShared_ThreadPresent(rrlog, entry[i].threadId)) {
		RRShared_SetupThread(rrlog, entry[i].threadId);
	    }
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

	    numEntries = (uint64_t)result / sizeof(RRLogEntry);
	}

	for (uint64_t i = 0; i < numEntries; i++) {
	    RRGlobalQueue_Append(&rrgq, &entries[i]);
	    if (entries[i].event == RREVENT_EXIT) {
		//printf("RXGQ Done\n");
		return NULL;
	    }
	}
    }
}

int
FillGlobalQueue()
{
    int result;
    uint64_t numEntries = 64;
    RRLogEntry entries[64];

    result = SystemRead(logfd, &entries, numEntries * sizeof(RRLogEntry));
    if (result < 0) {
	perror("read");
	return -1;
    }
    if (result == 0) {
	return 0;
    }

    numEntries = (uint64_t)result / sizeof(RRLogEntry);

    for (uint64_t i = 0; i < numEntries; i++) {
	RRGlobalQueue_Append(&rrgq, &entries[i]);
    }

    return numEntries;
}

int
QueueOne()
{
    uint64_t numEntries = 1;
    RRLogEntry *entry;

    if (RRGlobalQueue_Length(&rrgq) < 10) {
	FillGlobalQueue();
    }

    entry = RRGlobalQueue_Dequeue(&rrgq, &numEntries);
    if (numEntries == 0)
	return 0;

    RRPlay_AppendThread(rrlog, entry);
    RRGlobalQueue_Free(&rrgq, 1);

    return 1;
}

void
dumpEntry(RRLogEntry *entry)
{
    int i;
    const char *evtStr = "UNKNOWN";

    for (i = 0; rreventTable[i].str != 0; i++) {
	if (rreventTable[i].evtid == entry->event) {
	    evtStr = rreventTable[i].str;
	    break;
	}
    }

    printf("%016ld  %08x  %-16s  %016lx  %016lx  %016lx  %016lx  %016lx  %016lx\n",
	    entry->eventId, entry->threadId, evtStr, entry->objectId,
	    entry->value[0], entry->value[1], entry->value[2],
	    entry->value[3], entry->value[4]);
}

void
DumpLog()
{
    uint64_t len;
    RRLogEntry *entry;

    printf("Next Event: %08ld\n", rrlog->nextEvent);
    printf("Last Event: %08ld\n", rrlog->lastEvent);

    for (uint32_t i = 0; i < RRLOG_MAX_THREADS; i++) {
	RRLogThread *rrthr = RRShared_LookupThread(rrlog, i);
	if (RRShared_ThreadPresent(rrlog, i)) {
	    printf("Thread %02d:\n", i);
	    printf("  Offsets Free: %02ld Used: %02ld\n",
		    rrthr->freeOff, rrthr->usedOff);
	    printf("  Status: %016lx\n", rrthr->status);
	}
    }

    printf("GlobalQueue:\n");
    printf("%-16s  %-8s  %-16s  %-16s  %-16s  %-16s  %-16s  %-16s  %-16s\n",
	    "Event #", "Thread #", "Event", "Object ID",
	    "Value[0]", "Value[1]", "Value[2]", "Value[3]", "Value[4]");

    len = RRGlobalQueue_Length(&rrgq);
    if (len > 10)
	len = 10;

    entry = RRGlobalQueue_Dequeue(&rrgq, &len);
    printf("Head: %08ld Tail: %08ld\n", rrgq.head, rrgq.tail);
    for (uint64_t i = 0; i < len; i++) {
	dumpEntry(&entry[i]);
    }
}

void
OpenLog(const char *logfile, uintptr_t regionSz, uint32_t numEvents, bool forRecord)
{
    // Open log file
    int flags = O_RDWR;
    if (forRecord) {
	flags |= O_CREAT|O_TRUNC;
    }

    logfd = open(logfile, flags, 0600);
    if (logfd < 0) {
	char str[255];
	snprintf(str,255,"Could not open log the file: %s",logfile);
	perror(str);
	abort();
    }

    // Setup shared memory region
    key_t shmkey = ftok(logfile, 0);
    if (shmkey == -1) {
	perror("ftok");
	abort();
    }

    shmid = shmget(shmkey, RRLOG_DEFAULT_REGIONSZ, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (shmid == -1) {
	perror("shmget");
	abort();
    }

    rrlog = shmat(shmid, NULL, 0);
    if (rrlog == MAP_FAILED) {
	perror("shmat");
	abort();
    }

    RRLog_Init(rrlog, numEvents);
    rrlog->regionSz = regionSz;
    RRGlobalQueue_Init(&rrgq);
}

void
LogDone()
{
    pthread_join(rrthr, NULL);
    pthread_join(gqthr, NULL);
    shmdt(rrlog);
    shmctl(shmid, IPC_RMID, NULL);
}

void
RecordLog()
{
    pthread_create(&rrthr, NULL, DrainQueue, NULL);
    pthread_create(&gqthr, NULL, TXGQProc, NULL);
}

void
ReplayLog()
{
    pthread_create(&rrthr, NULL, FeedQueue, NULL);
    pthread_create(&gqthr, NULL, RXGQProc, NULL);
}


