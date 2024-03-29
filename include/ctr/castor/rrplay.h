
#ifndef __CASTOR_RRPLAY_H__
#define __CASTOR_RRPLAY_H__

#include "rrlog.h"

/*
 * Replay Log - Single-Writer Multiple-Reader Queue
 */

#define RRPlay_Init RRLog_Init

static inline void
RRPlay_AppendThread(RRLog *rrlog, RRLogEntry *entry)
{
    RRLogThread *rrthr = RRShared_LookupThread(rrlog, entry->threadId); 
    volatile RRLogEntry *rrentry = &rrthr->entries[rrthr->freeOff % rrlog->numEvents];

    while ((rrthr->freeOff - rrthr->usedOff) >= (rrlog->numEvents - 1)) {
	__asm__ volatile("pause\n");
    }

    rrentry->eventId = entry->eventId;
    rrentry->objectId = entry->objectId;
    rrentry->event = entry->event;
    rrentry->threadId = entry->threadId;
    rrentry->value[0] = entry->value[0];
    rrentry->value[1] = entry->value[1];
    rrentry->value[2] = entry->value[2];
    rrentry->value[3] = entry->value[3];
    rrentry->value[4] = entry->value[4];

    rrthr->freeOff += 1;

    return;
}

static inline RRLogEntry *
RRPlayThreadDequeue(RRLog *rrlog, RRLogThread *rrthr)
{
    volatile RRLogEntry *entry = &rrthr->entries[rrthr->usedOff % rrlog->numEvents];

    while (rrthr->freeOff == rrthr->usedOff) {
	__asm__ volatile("pause\n");
    }

    return (RRLogEntry *)entry;
}

static inline RRLogEntry *
RRPlay_Dequeue(RRLog *rrlog, uint32_t threadId)
{
    RRLogThread *rrthr = RRShared_LookupThread(rrlog, threadId); 
    RRLogEntry *entry = RRPlayThreadDequeue(rrlog, rrthr);

    while (entry->eventId != rrlog->nextEvent) {
	__asm__ volatile("pause\n");
    }

#ifdef RRLOG_DEBUG
    if (rrthr->status != 0) {
	__castor_abort();
    }
    rrthr->status = 1;
#endif /* RRLOG_DEBUG */

    return entry;
}

static inline void
RRPlay_Free(RRLog *rrlog, RRLogEntry *entry)
{
    RRLogThread *rrthr = RRShared_LookupThread(rrlog, entry->threadId); 
#ifdef RRLOG_DEBUG
    rrthr->status = 0;
#endif /* RRLOG_DEBUG */

    __sync_add_and_fetch(&rrlog->nextEvent, 1);
    rrthr->usedOff += 1;
}

#endif /* __CASTOR_RRPLAY_H__ */

