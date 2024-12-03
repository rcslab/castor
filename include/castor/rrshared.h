
#ifndef __CASTOR_RRSHARED_H__
#define __CASTOR_RRSHARED_H__

#include <assert.h>

#ifdef CASTOR_RUNTIME
#include <castor/rr_debug.h>
#undef assert
#define assert rr_assert
#endif

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

#include "debug.h"
#include "mtx.h"

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
    /* Offset to RRLogThread struct. */
    atomic_uintptr_t				offset; 
    /* The real pid of the running process. */
    int						pid;
    int						tid;
    /* The pid used during record phase. Used in replay only. */
    int						recordedPid; 
} RRLogThreadInfo;

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

static inline bool
RRShared_ThreadPresent(RRLog *rrlog, uint32_t threadId)
{

    return rrlog->threadInfo[threadId].offset != 0;
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
    uintptr_t offset = RRShared_AllocRegion(rrlog, RRShared_ThreadSize(rrlog));
    RRLogThread *thr = (RRLogThread *)((uintptr_t)rrlog + offset);
    uintptr_t expected = 0;

    //LOG("SetupThread %p", thr);

    thr->freeOff = 0;
    thr->usedOff = 0;
    thr->status = 0;
    for (uint64_t i = 0; i < rrlog->numEvents; i++) {
	memset((void *)&thr->entries[i], 0, sizeof(RRLogEntry));
    }

    if (!atomic_compare_exchange_weak(&rrlog->threadInfo[tid].offset, &expected, offset)) {
	return;
    } else {
	//LOG("SetupThread Fail %d, %p", tid, RRShared_LookupThread(rrlog, 
	//tid));
	return;
    }
}

static inline uint32_t
RRShared_AllocThread(RRLog *rrlog)
{
    uintptr_t offset = RRShared_AllocRegion(rrlog, RRShared_ThreadSize(rrlog));
    RRLogThread *thr = (RRLogThread *)((uintptr_t)rrlog + offset);

    //LOG("AllocThread %p", thr);

    thr->freeOff = 0;
    thr->usedOff = 0;
    thr->status = 0;
    for (uint64_t i = 0; i < rrlog->numEvents; i++) {
	memset((void *)&thr->entries[i], 0, sizeof(RRLogEntry));
    }

    for (uint32_t i = 0; i < RRLOG_MAX_THREADS; i++) {
	uintptr_t expected = 0;

	if (atomic_compare_exchange_weak(&rrlog->threadInfo[i].offset, &expected, offset)) {
	    //LOG("LookupThread %p", RRShared_LookupThread(rrlog, i));
	    return i;
	}
    }

    return 0;
}

#endif /* __CASTOR_RRSHARED_H__ */

