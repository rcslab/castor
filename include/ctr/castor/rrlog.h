
#ifndef __CASTOR_RRLOG_H__
#define __CASTOR_RRLOG_H__

#include <assert.h>
#include <stdint.h>
#include <stdalign.h>
#include <stdlib.h>

#include <castor/archconfig.h>
#include <castor/rrshared.h>

#ifdef CASTOR_DEBUG
#define RRLOG_DEBUG		1
#endif

/*
 * Record Log - Multiple-Writer Single-Reader Queue
 */
static inline void
RRLog_Init(RRLog *rrlog, uint32_t numEvents)
{
    assert((uintptr_t)rrlog % PAGESIZE == 0);

    rrlog->nextEvent = 1;
    rrlog->lastEvent = 0;
    atomic_init(&rrlog->freePtr, ROUNDUP(sizeof(*rrlog), PAGESIZE));
    rrlog->rrMode = RRMODE_NORMAL;
    rrlog->numThreads = RRLOG_MAX_THREADS;
    rrlog->numEvents = numEvents;

    for (int i = 0; i < RRLOG_MAX_THREADS; i++) {
	rrlog->threadInfo[i].offset = 0;
    }

    // System V
    rrlog->sysvmap = 0;
    Mutex_Init(&rrlog->sysvlck);
}

static inline RRLogEntry *
RRLog_Alloc(RRLog *rrlog, uint32_t threadId)
{
    RRLogThread *rrthr = RRShared_LookupThread(rrlog, threadId);
    volatile RRLogEntry *rrentry;

#ifdef RRLOG_DEBUG
    if (rrthr->status == 1) {
	abort();
    }
    rrthr->status = 1;
#endif /* RRLOG_DEBUG */

    rrentry = &rrthr->entries[rrthr->freeOff % rrlog->numEvents];

    while ((rrthr->freeOff - rrthr->usedOff) >= (rrlog->numEvents - 1))
    {
	__asm__ volatile("pause\n");
    }

    rrentry->threadId = threadId;

    return (RRLogEntry *)rrentry;
}

static inline void
RRLog_Append(RRLog *rrlog, RRLogEntry *rrentry)
{
    RRLogThread *rrthr = RRShared_LookupThread(rrlog, rrentry->threadId);

    rrentry->eventId = __sync_add_and_fetch(&rrlog->lastEvent, 1);
    rrthr->freeOff += 1;

#ifdef RRLOG_DEBUG
    rrthr->status = 0;
#endif /* RRLOG_DEBUG */
}

static inline RRLogEntry *
RRLogThreadDequeue(RRLog *rrlog, RRLogThread *rrthr)
{
    volatile RRLogEntry *entry = &rrthr->entries[rrthr->usedOff % rrlog->numEvents];

    if (rrthr->freeOff == rrthr->usedOff)
	return NULL;

    return (RRLogEntry *)entry;
}

static inline RRLogEntry *
RRLog_Dequeue(RRLog *rrlog)
{
    uint64_t nextEvent = rrlog->nextEvent;
    RRLogEntry *entry;

    for (uint32_t i = 0; i < RRLOG_MAX_THREADS; i++) {
	if (rrlog->threadInfo[i].offset != 0) {
	    RRLogThread *rrthr = RRShared_LookupThread(rrlog, i);

	    entry = RRLogThreadDequeue(rrlog, rrthr);

	    if (entry && entry->eventId == nextEvent) {
		rrlog->nextEvent += 1;
		return entry;
	    }
	}
    }

    return NULL;
}

static inline void
RRLog_Free(RRLog *rrlog, RRLogEntry *rrentry)
{
    RRLogThread *rrthr = RRShared_LookupThread(rrlog, rrentry->threadId);
    rrthr->usedOff += 1;
}

#endif /* __CASTOR_RRLOG_H__ */

