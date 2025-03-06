
#ifndef __CASTOR_RRSHARED_H__
#define __CASTOR_RRSHARED_H__

#include <assert.h>

#ifdef CASTOR_RUNTIME
#include <castor/rr_debug.h>
#undef assert
#define assert rr_assert
#endif

#include <sys/syscall.h>
#include <unistd.h>

#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
#include <atomic>
using namespace std;
#else
#include <stdatomic.h>
#endif
#include <string.h>
#include <stdlib.h>

#if defined(__amd64__)
#include <immintrin.h>
#endif

#include "debug.h"
#include "mtx.h"
#include "hash.h"

enum RRMODE {
    RRMODE_NORMAL, // Normal
    RRMODE_RECORD, // Debug Recording
    RRMODE_REPLAY, // Replay
    //RRMODE_FASTRECORD, // Fault Tolerance Recording
    //RRMODE_FASTREPLAY, // Fault Tolerance Replay
};

// Castor Tunables
#define RRLOG_MAX_THREADS	128
#define RRLOG_DEFAULT_REGIONSZ	(64*1024*1024)
#define RRLOG_DEFAULT_EVENTS	((PAGESIZE-4*CACHELINE)/CACHELINE)

typedef struct RRLogEntry {
    alignas(CACHELINE) uint64_t		eventId;
    uint64_t				objectId;
    uint32_t				event;
    uint32_t				threadId;
    uint64_t				value[5];
} RRLogEntry;

static_assert(alignof(RRLogEntry) == CACHELINE, "RRLogEntry must be cache line aligned");
static_assert(sizeof(RRLogEntry) == CACHELINE, "RRLogEntry must be cache line sized");

typedef struct RRLogThread {
    alignas(CACHELINE) volatile uint64_t	freeOff; // Next free entry
    alignas(CACHELINE) volatile uint64_t	usedOff; // Next entry to consume
    alignas(CACHELINE) volatile uint64_t	status; // Debugging
    alignas(CACHELINE) volatile uint64_t	_rsvd;
    alignas(CACHELINE) volatile RRLogEntry	entries[];
} RRLogThread;

static_assert(alignof(RRLogThread) == CACHELINE, "RRLogThread must be page aligned");

typedef struct RRLogThreadInfo {
    atomic_uintptr_t				enabled;
    /* Offset to RRLogThread struct. */
    atomic_uintptr_t				offset; 
    /* The real pid of the running process. */
    int						pid;
    int						tid;
    /* The pid used during record phase. Used in replay only. */
    int						recordedPid; 
} RRLogThreadInfo;

enum RRSyncType {
    RRSync_Null = 0,
    RRSync_Mutex,
    RRSync_Spinlock,
    RRSync_RWLock
};

#ifndef atomic_uint64_t
typedef atomic_uint_fast64_t atomic_uint64_t;
#endif

typedef struct RRSyncEntry {
    atomic_uintptr_t addr;
    uintptr_t type;
    atomic_uint64_t epoch;
    uint64_t owner;
} RRSyncEntry;

#define RRLOG_SYNCTABLE_SIZE 4096

typedef struct RRSyncTable {
    RRSyncEntry entries[RRLOG_SYNCTABLE_SIZE];
} RRSyncTable;

#define RRLOG_MAGIC		0x436173746f724654

typedef struct RRLog {
    uint64_t magic;				    // Magic
    alignas(CACHELINE) atomic_uintptr_t dbgwait;    // Wait for debugger
    alignas(CACHELINE) volatile uint64_t nextEvent; // Next event to be read
    alignas(CACHELINE) volatile uint64_t lastEvent; // Last event to be allocated
    alignas(CACHELINE) atomic_uintptr_t freePtr;    // Next Free Region (4KB aligned)
    enum RRMODE rrMode;				    // R/R Mode
    uintptr_t regionSz;				    // Region Size
    uint64_t numThreads;			    // Max number of threads
    uint64_t numEvents;				    // Max number of events
    RRLogThreadInfo threadInfo[RRLOG_MAX_THREADS];
    Mutex threadInfoMtx;
    RRSyncTable syncTable;
    atomic_uintptr_t sysvmap;
    Mutex sysvlck;
} RRLog;

extern uint64_t getThreadId();
extern uint64_t getRecordedPid();
extern void setRecordedPid(uint64_t);
extern void setThreadId(uint64_t, uint64_t);

#define ROUNDUP(_x, _n) ((_x + _n - 1) & ~(_n - 1))

static inline uintptr_t
RRShared_AllocRegion(RRLog *rrlog, uintptr_t sz)
{
    uintptr_t finalSz = ROUNDUP(sz, PAGESIZE);
    uintptr_t offset = atomic_fetch_add(&rrlog->freePtr, finalSz);

    assert(offset < rrlog->regionSz);

    return offset;
}

static inline int
RRShared_FindRealPidFromRecordedPid(RRLog *rrlog, int rpid)
{
    int pid = -1;

    for (int i=0;i<RRLOG_MAX_THREADS;i++) {
	if (rrlog->threadInfo[i].recordedPid != rpid)
	    continue;

	pid = rrlog->threadInfo[i].pid;
	break;
    }

    assert(pid != -1);
    return pid;
}

static inline bool
RRShared_CheckThreadReusable(RRLog *rrlog, uint32_t threadId)
{
    return rrlog->threadInfo[threadId].offset != 0 && rrlog->threadInfo[threadId].enabled == 0;
}

static inline bool
RRShared_ThreadPresent(RRLog *rrlog, uint32_t threadId)
{
    return rrlog->threadInfo[threadId].offset != 0 && rrlog->threadInfo[threadId].enabled == 1;
}

static inline RRLogThread *
RRShared_LookupThread(RRLog *rrlog, uint32_t threadId)
{
    return (RRLogThread *)((uintptr_t)rrlog + rrlog->threadInfo[threadId].offset);
}

static inline uintptr_t
RRShared_ThreadSize(RRLog *rrlog)
{
    return sizeof(RRLogThread) + sizeof(RRLogEntry) * rrlog->numEvents;
}

static inline void
RRShared_SetupThread(RRLog *rrlog, uint32_t tid)
{
    uintptr_t offset;
    int reused = 0;
    RRLogThread *thr;

    Mutex_Lock(&rrlog->threadInfoMtx);

    if (RRShared_ThreadPresent(rrlog, tid)) {
	Mutex_Unlock(&rrlog->threadInfoMtx);
	return;
    }

    if (RRShared_CheckThreadReusable(rrlog, tid)) {
	offset = rrlog->threadInfo[tid].offset;
	reused = 1;
    } else {
	offset = RRShared_AllocRegion(rrlog, RRShared_ThreadSize(rrlog));
    }
    thr = (RRLogThread *)((uintptr_t)rrlog + offset);

    /*
     * Keep the pointer and entry for reused threadInfo as the these info were
     * loaded already.
     */
    thr->status = 0;
    if (!reused) {
	thr->freeOff = 0;
	thr->usedOff = 0;
	for (uint64_t i = 0; i < rrlog->numEvents; i++) {
	    memset((void *)&thr->entries[i], 0, sizeof(RRLogEntry));
	}
    }

    rrlog->threadInfo[tid].enabled = 1;
    rrlog->threadInfo[tid].offset = offset;

    Mutex_Unlock(&rrlog->threadInfoMtx);
    return;
}

static inline uint32_t
RRShared_AllocThread(RRLog *rrlog)
{
    uintptr_t offset = 0;
    RRLogThread *thr; 
    int thrNum = -1;

    Mutex_Lock(&rrlog->threadInfoMtx);

    for (int i=0;i<RRLOG_MAX_THREADS;i++) {
	if (!RRShared_CheckThreadReusable(rrlog, i))
	    continue;

	rrlog->threadInfo[i].enabled = 1;
	offset = rrlog->threadInfo[i].offset;
	thrNum = i;
	break;
    }

    if (!offset) {
	offset = RRShared_AllocRegion(rrlog, RRShared_ThreadSize(rrlog));
    }

    thr = (RRLogThread *)((uintptr_t)rrlog + offset);
    thr->freeOff = 0;
    thr->usedOff = 0;
    thr->status = 0;
    for (uint64_t i = 0; i < rrlog->numEvents; i++) {
	memset((void *)&thr->entries[i], 0, sizeof(RRLogEntry));
    }

    if (thrNum == -1) {
	for (uint32_t i = 0; i < RRLOG_MAX_THREADS; i++) {
	    if (rrlog->threadInfo[i].enabled || rrlog->threadInfo[i].offset)
		continue;

	    rrlog->threadInfo[i].enabled = 1;
	    rrlog->threadInfo[i].offset = offset;
	    thrNum = i;
	    break;
	}
    }

    assert(thrNum != -1);
    Mutex_Unlock(&rrlog->threadInfoMtx);

    return thrNum;
}

/*
 * The parameter pid is the real pid.
 * If parameter pid is 0, all threads are cleaned.
 */
static inline void
RRShared_CleanThread(RRLog *rrlog, uint32_t pid)
{
    Mutex_Lock(&rrlog->threadInfoMtx);

    for (int i = 0;i < RRLOG_MAX_THREADS;i++) {
	if (rrlog->threadInfo[i].pid != (int)pid && pid != 0)
	    continue;
	
	rrlog->threadInfo[i].enabled = 0;
    }
    Mutex_Unlock(&rrlog->threadInfoMtx);
}

/*
 * Debugging purpose, disable in release
 */
static inline void
RRShared_AssertThreadAtExit(RRLog *rrlog)
{
#ifdef CASTOR_DEBUG
    int count = 0;
    for (int i = 0;i < RRLOG_MAX_THREADS;i++) {
	if (rrlog->threadInfo[i].enabled == 0)
	    continue;

	if (rrlog->threadInfo[i].enabled) {
	    count++;
	}

	ASSERT(count == 0);
    }
#endif
}

// XXX: Add a way to destroy entries
static inline RRSyncEntry *
RRShared_SyncLookup(RRLog *rrlog, uintptr_t addr, uint64_t type)
{
    int bStart = Hash_Mix64(addr) % RRLOG_SYNCTABLE_SIZE;
    int b = bStart;

    while (1) {
	if (rrlog->syncTable.entries[b].addr == addr) {
#ifdef CASTOR_DEBUG
	    uint64_t stype = rrlog->syncTable.entries[b].type;
	    /*
	     * Either it should be the right type or null if racing with 
	     * allocation.
	     */
	    ASSERT(stype == type || stype == 0);
#endif
	    return &(rrlog->syncTable.entries[b]);
	}

	if (rrlog->syncTable.entries[b].addr == 0) {
	    uintptr_t v = 0;
	    atomic_compare_exchange_strong(&rrlog->syncTable.entries[b].addr, &v, addr);
	    rrlog->syncTable.entries[b].type = type;
	    continue;
	}

	b = (b + 1) % RRLOG_SYNCTABLE_SIZE;

	// Out of space
	if (b == bStart) {
	    PANIC();
	}
    }
}

static inline void
RRShared_SyncWait(RRSyncEntry *s, uint64_t epoch)
{
    uint64_t count = 0;

    while (epoch != atomic_load(&s->epoch)) {
#if defined(__amd64__)
	_mm_pause();
#endif
	if (count++ > 50) {
	    syscall(SYS_sched_yield);
	}
    }
}

static inline void
RRShared_SyncInc(RRSyncEntry *s)
{
    s->owner = getThreadId();
    atomic_fetch_add(&s->epoch, 1);
}

#endif /* __CASTOR_RRSHARED_H__ */

