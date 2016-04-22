
#ifndef __RRLOG_H__
#define __RRLOG_H__

#include <assert.h>
#include <stdint.h>
#include <stdalign.h>

#ifdef CASTOR_DEBUG
#define RRLOG_DEBUG		1
#endif

// XXX: Get this from architecture specific headers
#define PAGESIZE		4096
#define CACHELINE		64

// Tunables
#define RRLOG_MAX_THREADS	8
#define RRLOG_MAX_ENTRIES	(PAGESIZE / CACHELINE - 4)

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
    alignas(CACHELINE) volatile RRLogEntry	entries[RRLOG_MAX_ENTRIES];
} RRLogThread;

static_assert(alignof(RRLogThread) == CACHELINE, "RRLogThread must be page aligned");
static_assert(sizeof(RRLogThread) == PAGESIZE, "RRLogThread must be page sized");

typedef struct RRLog {
    alignas(CACHELINE) volatile uint64_t nextEvent; // Next event to be read
    alignas(CACHELINE) volatile uint64_t lastEvent; // Last event to be allocated
    alignas(PAGESIZE) RRLogThread threads[RRLOG_MAX_THREADS];
} RRLog;

static_assert(alignof(RRLog) == PAGESIZE, "RRLog must be page aligned");

/*
 * Record Log - Multiple-Writer Single-Reader Queue
 */
static void
RRLog_Init(RRLog *rrlog)
{
    int i;
    int j;

    assert((uintptr_t)rrlog % PAGESIZE == 0);

    for (i = 0; i < RRLOG_MAX_THREADS; i++) {
	rrlog->threads[i].freeOff = 0;
	rrlog->threads[i].usedOff = 0;
	for (j = 0; j < RRLOG_MAX_ENTRIES; j++) {
	    rrlog->threads[i].entries[j].eventId = 0;
	    rrlog->threads[i].entries[j].objectId = 0;
	    rrlog->threads[i].entries[j].event = 0;
	    rrlog->threads[i].entries[j].threadId = i;
	}
    }

    rrlog->nextEvent = 1;
    rrlog->lastEvent = 0;
}

static inline RRLogEntry *
RRLog_Alloc(RRLog *rrlog, uint32_t threadId)
{
    RRLogThread *rrthr = &rrlog->threads[threadId];
    volatile RRLogEntry *rrentry;

#ifdef RRLOG_DEBUG
    if (rrthr->status == 1) {
	abort();
    }
    rrthr->status = 1;
#endif /* RRLOG_DEBUG */

    rrentry = &rrthr->entries[rrthr->freeOff % RRLOG_MAX_ENTRIES];

    while (rrthr->freeOff - rrthr->usedOff >= (RRLOG_MAX_ENTRIES - 1))
    {
	__asm__ volatile("pause\n");
    }

    return (RRLogEntry *)rrentry;
}

static inline void
RRLog_Append(RRLog *rrlog, RRLogEntry *rrentry)
{
    rrentry->eventId = __sync_add_and_fetch(&rrlog->lastEvent, 1);
    rrlog->threads[rrentry->threadId].freeOff += 1;

#ifdef RRLOG_DEBUG
    rrlog->threads[rrentry->threadId].status = 0;
#endif /* RRLOG_DEBUG */
}

static inline RRLogEntry *
RRLogThreadDequeue(RRLogThread *rrthr)
{
    volatile RRLogEntry *entry = &rrthr->entries[rrthr->usedOff % RRLOG_MAX_ENTRIES];

    if (rrthr->freeOff == rrthr->usedOff)
	return NULL;

    return (RRLogEntry *)entry;
}

static inline RRLogEntry *
RRLog_Dequeue(RRLog *rrlog)
{
    int i;
    uint64_t nextEvent = rrlog->nextEvent;
    RRLogEntry *entry;

    for (i = 0; i < RRLOG_MAX_THREADS; i++) {
	entry = RRLogThreadDequeue(&rrlog->threads[i]);

	if (entry && entry->eventId == nextEvent) {
	    rrlog->nextEvent += 1;
	    return entry;
	}
    }

    return NULL;
}

static inline void
RRLog_Free(RRLog *rrlog, RRLogEntry *rrentry)
{
    rrlog->threads[rrentry->threadId].usedOff += 1;
}

#endif /* __RRLOG_H__ */

