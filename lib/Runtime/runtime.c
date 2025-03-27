
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdlib.h>

#include <threads.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/capsicum.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/shm.h>

#include <libc_private.h>

#include <castor/debug.h>
#include <castor/rr_debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "system.h"
#include "util.h"

extern char * __castor_getenv(const char * name);

extern interpos_func_t __libc_interposing[] __hidden;
extern int __rr_sigaction(int sig, const struct sigaction *restrict act, 
    struct sigaction *restrict oact);

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

RRLog *rrlog;
thread_local enum RRMODE rrMode = RRMODE_NORMAL;

int
getRRMode()
{
    return rrMode;
}

void 
setRRMode(int mode)
{
    rrMode = mode;
}

static int
LookupThreadId()
{
    int pid;
    long tid;

    pid = __rr_syscall(SYS_getpid);
    __rr_syscall(SYS_thr_self, &tid);

    for (int i = 0; i < RRLOG_MAX_THREADS; i++) {
	if (rrlog->threadInfo[i].pid == pid &&
	    rrlog->threadInfo[i].tid == tid)
	    return i;
    }

    return -1;
}

static void
event_init()
{
    *(__libc_interposing_slot(INTERPOS_sigaction)) = (interpos_func_t)&__rr_sigaction;
}

void
dumpEntry(volatile RRLogEntry *entry)
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
dumpLog()
{
    int thr_entry_st, thr_entry_ed;

    LOG("Next Event: %08ld", rrlog->nextEvent);
    LOG("Last Event: %08ld", rrlog->lastEvent);

    LOG("ThreadQueue:");
    for (int i=0; i < RRLOG_MAX_THREADS; i++) {
	RRLogThread *rrthr = RRShared_LookupThread(rrlog, i);
	if (!RRShared_ThreadPresent(rrlog, i))
	    continue;

	LOG("Thread %d", i);
	LOG("  Offsets Free: %02ld Used: %02ld",
	    rrthr->freeOff, rrthr->usedOff);
	LOG("  Status: %016lx", rrthr->status);
	LOG("%-16s  %-8s  %-16s  %-16s  %-16s  %-16s  %-16s  %-16s  %-16s",
	    "Event #", "Thread #", "Event", "Object ID",
	    "Value[0]", "Value[1]", "Value[2]", "Value[3]", "Value[4]");

	if (rrthr->freeOff > rrlog->numEvents)
	    thr_entry_st = rrthr->freeOff - rrlog->numEvents;
	else
	    thr_entry_st = 0;
	thr_entry_ed = rrthr->freeOff;
	for (int j=thr_entry_st; j<thr_entry_ed;j++) {
	    dumpEntry(&rrthr->entries[j % rrlog->numEvents]);
	}
	LOG("\n");
    }

    LOG("SyncTable:");
    LOG("%-16s  %-8s  %-16s  %-16s", "Address", "Type", "Epoch", "Owner");
    for (int i=0; i < RRLOG_SYNCTABLE_SIZE; i++) {
	const char *Types[] = { "Null", "Mutex", "Spinlock", "RWLock" };
	RRSyncEntry *s = &rrlog->syncTable.entries[i];

	if (s->addr != 0) {
	    LOG("%016lx  %-8s  %016lx  %016lx",
		s->addr, Types[s->type], s->epoch, s->owner);
	}
    }
}

extern void Debug_Sighandler(int signal);

void
dump_siginfo(int signal)
{
    dumpLog();
}

void
log_init()
{
    int status;
    RRLogEntry *e;

    castor_dummy();
    if (rrMode != RRMODE_NORMAL)
	return;

    char *shmpath = __castor_getenv("CASTOR_SHMPATH");
    if (shmpath == NULL) {
	return;
    }

    char *mode = __castor_getenv("CASTOR_MODE");
    if (mode == NULL) {
	rrMode = RRMODE_NORMAL;
	return;
    }

    Debug_Init("castor.log");
    signal(SIGINFO, dump_siginfo);

    LOG("shmpath = %s", shmpath);
    LOG("mode = %s", mode);

    key_t shmkey = __castor_ftok(shmpath, 0);
    if (shmkey == -1) {
	PERROR("ftok");
    }

    LOG("shmkey = %ld", shmkey);

    int shmid = __rr_syscall(SYS_shmget, shmkey, RRLOG_DEFAULT_REGIONSZ, 0);
    if (shmid == -1) {
	PERROR("shmget");
    }

    rrlog = (RRLog *) __rr_syscall_long(SYS_shmat, shmid, NULL, 0);
    if (rrlog == MAP_FAILED) {
	PERROR("shmat");
    }

    int thrNo = LookupThreadId();
    if (thrNo == -1) {
	RRShared_SetupThread(rrlog, 0);
	setThreadId(0, -1);
    } else {
	setThreadId(thrNo, rrlog->threadInfo[thrNo].recordedPid);
    }

    // XXX: Need to remap the region again to the right size
    ASSERT(RRLOG_DEFAULT_REGIONSZ == rrlog->regionSz);

    char *sandbox = __castor_getenv("CASTOR_SANDBOX");
    if (sandbox) {
	status = __rr_syscall(SYS_cap_enter);
	if (status != 0) {
	    PERROR("cap_enter");
	}
    }

    event_init();

    char *stopchild = __castor_getenv("CASTOR_STOPCHILD");
    if (stopchild && strcmp(stopchild,"1") == 0) {
	while (atomic_load(&rrlog->dbgwait) != 1)
	    ;
    }

    if (strcmp(mode, "RECORD") == 0) {
	rr_fdprintf(STDERR_FILENO, "RECORD\n");
	rrMode = RRMODE_RECORD;
    } else if (strcmp(mode, "REPLAY") == 0) {
	rr_fdprintf(STDERR_FILENO, "REPLAY\n");
	rrMode = RRMODE_REPLAY;
    }

    /* Always generate procinfo event for the forked process */
    if (thrNo == -1) {
	rrlog->threadInfo[0].rrMode = rrMode;

	switch (rrMode) {
	case RRMODE_RECORD:
	    e = RRLog_Alloc(rrlog, 0);
	    e->event = RREVENT_PROCINFO;
	    e->value[0] = __rr_syscall(SYS_getpid);
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, 0);
	    AssertEvent(e, RREVENT_PROCINFO);
	    setRecordedPid(e->value[0]);
	    RRPlay_Free(rrlog, e);
	    break;
	default:
	    /* Do nothing. */
	    break;
	}
    } else {
	rrlog->threadInfo[thrNo].rrMode = rrMode;
    }
}

