
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
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
#include <castor/file_format.h>
#include <castor/Common/runtime.h>
#include <castor/Common/ft.h>

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

void
EnablePThreadCancel()
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
}

void *
DrainQueue(void *arg)
{
    uint64_t procs = 1;
#if !(defined(CASTOR_CTR) || defined(CASTOR_TSC))
    uint64_t i = 0;
#endif

    EnablePThreadCancel();

    while (1) {
	RRLogEntry *entry = NULL;

#if defined(CASTOR_CTR) || defined(CASTOR_TSC)
	do {
	    entry = RRLog_Dequeue(rrlog);
	} while (entry == NULL);
#elif defined(CASTOR_TSX)

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

	if (entry->event == RREVENT_FORK) {
	    procs++;
	} else if (entry->event == RREVENT_EXIT) {
	    procs--;
	    if (procs == 0) {
		RRLog_Free(rrlog, entry);
		break;
	    }
	}

	RRLog_Free(rrlog, entry);
    }

    //printf("Consumer Done\n");
    return NULL;
}

void *
TXGQProc(void *arg)
{
    uint64_t procs = 1;

    EnablePThreadCancel();

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
	    if (entry[i].event == RREVENT_FORK) {
		procs++;
	    } else if (entry[i].event == RREVENT_EXIT) {
		procs--;
		if (procs == 0) {
		    printf("TXGQ Done\n");
		    return NULL;
		}
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
    uint64_t procs = 1;

    EnablePThreadCancel();

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
	    if (entry[i].event == RREVENT_FORK) {
		procs++;
	    } else if (entry[i].event == RREVENT_EXIT) {
		procs--;
		if (procs == 0) {
		    //printf("Feed Done\n");
		    pthread_exit(NULL);
		    __builtin_unreachable();
		}
	    }
	}

	RRGlobalQueue_Free(&rrgq, numEntries);
    }
}

void *
RXGQProc(void *arg)
{
    uint64_t procs = 1;

    EnablePThreadCancel();

    while (1) {
	uint64_t numEntries = 512;
	RRLogEntry entries[512];

	if (ftMode) {
	    numEntries = RRFT_Recv(numEntries, (RRLogEntry *)&entries);
	} else {
	    int result = SystemRead(logfd, &entries, numEntries * sizeof(RRLogEntry));
	    if (result < 0) {
		PERROR("read");
	    }
	    if (result == 0) {
		return NULL;
	    }

	    numEntries = (uint64_t)result / sizeof(RRLogEntry);
	}

	for (uint64_t i = 0; i < numEntries; i++) {
	    RRGlobalQueue_Append(&rrgq, &entries[i]);
	    if (entries[i].event == RREVENT_FORK) {
		procs++;
	    } else if (entries[i].event == RREVENT_EXIT) {
		procs--;
		if (procs == 0)
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
	PERROR("read");
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
dumpEntryDebug(RRLogEntry *entry)
{
    int i;
    const char *evtStr = "UNKNOWN";

    for (i = 0; rreventTable[i].str != 0; i++) {
	if (rreventTable[i].evtid == entry->event) {
	    evtStr = rreventTable[i].str;
	    break;
	}
    }

    LOG("%016ld  %08x  %-16s  %016lx  %016lx  %016lx  %016lx  %016lx  %016lx",
	    entry->eventId, entry->threadId, evtStr, entry->objectId,
	    entry->value[0], entry->value[1], entry->value[2],
	    entry->value[3], entry->value[4]);
}

void
DumpLogDebug()
{
    uint64_t len;
    RRLogEntry *entry;

    LOG("Next Event: %08ld", rrlog->nextEvent);
    LOG("Last Event: %08ld", rrlog->lastEvent);

    for (uint32_t i = 0; i < RRLOG_MAX_THREADS; i++) {
	RRLogThread *rrthr = RRShared_LookupThread(rrlog, i);
	if (RRShared_ThreadPresent(rrlog, i)) {
	    LOG("Thread %02d:", i);
	    LOG("  Offsets Free: %02ld Used: %02ld",
		    rrthr->freeOff, rrthr->usedOff);
	    LOG("  Status: %016lx", rrthr->status);
	    for (uint64_t i = 0; i < rrlog->numEvents; i++) {
		dumpEntryDebug((RRLogEntry *)&rrthr->entries[i]);
	    }
	}
    }

    LOG("GlobalQueue:");
    LOG("%-16s  %-8s  %-16s  %-16s  %-16s  %-16s  %-16s  %-16s  %-16s",
	    "Event #", "Thread #", "Event", "Object ID",
	    "Value[0]", "Value[1]", "Value[2]", "Value[3]", "Value[4]");

    len = RRGlobalQueue_Length(&rrgq);
    if (len > 32)
	len = 32;

    entry = RRGlobalQueue_Dequeue(&rrgq, &len);
    LOG("Head: %08ld Tail: %08ld", rrgq.head, rrgq.tail);
    for (uint64_t i = 0; i < len && i < 128; i++) {
	dumpEntryDebug(&entry[i]);
    }
}

void
SignalCloseLog(int sig)
{
    int status;

    pthread_cancel(rrthr);
    pthread_cancel(gqthr);
    pthread_join(rrthr, NULL);
    pthread_join(gqthr, NULL);

    RRLog *rl = rrlog;
    if (rl != NULL) {
	status = shmdt(rrlog);
	if (status == -1) {
	    LOG("shmdt");
	}
	status = shmctl(shmid, IPC_RMID, NULL);
	if (status == -1) {
	    LOG("shmctl");
	}
    }

    LOG("Forced exiting of record");
    exit(1);
}

void
OpenLog(const char *logfile, uintptr_t regionSz, uint32_t numEvents, bool forRecord)
{
    castor_magic_t magic;
    castor_version_t version;

    int flags = O_RDWR;
    if (forRecord) {
	flags |= O_CREAT|O_TRUNC;
    }

    logfd = open(logfile, flags, 0600);
    if (logfd < 0) {
	fprintf(stderr, "Could not open record/replay log '%s'", logfile);
	PERROR("open");
    }

    if (forRecord) {
	magic = CASTOR_MAGIC_NUMBER;
	version = CASTOR_VERSION_NUMBER;
	write(logfd, &magic, sizeof(magic));
	write(logfd, &version, sizeof(version));
    } else {
	read(logfd, &magic, sizeof(magic));
	read(logfd, &version, sizeof(version));
	if (magic != CASTOR_MAGIC_NUMBER) {
	    fprintf(stderr, "OpenLog failed: %s does not appear to be a log file, invalid "\
			    "magic number at file start.\n", logfile);
	    exit(1);
	}
	if (version != CASTOR_VERSION_NUMBER) {
	    fprintf(stderr, "OpenLog failed: %s recorded with version %d,"\
			    "current version is %d.\n",
			    logfile, version, CASTOR_VERSION_NUMBER);
	    exit(1);
	}
    }

    signal(SIGINT, &SignalCloseLog);
    signal(SIGKILL, &SignalCloseLog);

    // Setup shared memory region
    key_t shmkey = ftok(logfile, 0);
    if (shmkey == -1) {
	PERROR("ftok");
    }

    LOG("shmkey = %ld", shmkey);

    shmid = shmget(shmkey, RRLOG_DEFAULT_REGIONSZ, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (shmid == -1) {
	PERROR("shmget");
    }

    rrlog = shmat(shmid, NULL, 0);
    if (rrlog == MAP_FAILED) {
	shmctl(shmid, IPC_RMID, NULL);
	PERROR("shmat");
    }

    RRLog_Init(rrlog, numEvents);
    rrlog->regionSz = regionSz;
    RRGlobalQueue_Init(&rrgq);
}

void
ResumeDebugWait()
{
    atomic_store(&rrlog->dbgwait, 1);
}

void
LogDone()
{
    int status;

    pthread_join(rrthr, NULL);
    pthread_join(gqthr, NULL);
    status = shmdt(rrlog);
    rrlog = NULL;
    if (status == -1) {
	PERROR("shmdt");
    }
    status = shmctl(shmid, IPC_RMID, NULL);
    if (status == -1) {
	PERROR("shmctl");
    }
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


