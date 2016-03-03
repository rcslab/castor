
#ifndef __RRPLAY_H__
#define __RRPLAY_H__

#include "rrlog.h"

/*
 * Replay Log - Single-Writer Multiple-Reader Queue
 */

#define RRPlay_Init RRLog_Init

static inline void
RRPlay_AppendThread(RRLog *rrlog, RRLogEntry *entry)
{
    RRLogThread *rrthr = &rrlog->threads[entry->threadId];
    volatile RRLogEntry *rrentry = &rrthr->entries[rrthr->freeOff % RRLOG_MAX_ENTRIES];

    while (rrthr->freeOff - rrthr->usedOff >= (RRLOG_MAX_ENTRIES - 1)) {
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
RRPlayThreadDequeue(RRLogThread *rrthr)
{
    volatile RRLogEntry *entry = &rrthr->entries[rrthr->usedOff % RRLOG_MAX_ENTRIES];

    while (rrthr->freeOff == rrthr->usedOff) {
	__asm__ volatile("pause\n");
    }

    return (RRLogEntry *)entry;
}

static inline RRLogEntry *
RRPlay_Dequeue(RRLog *rrlog, uint32_t threadId)
{
    RRLogThread *rrthr = &rrlog->threads[threadId];
    RRLogEntry *entry = RRPlayThreadDequeue(rrthr);

    while (entry->eventId != rrlog->nextEvent) {
	__asm__ volatile("pause\n");
    }

    return entry;
}

static inline void
RRPlay_Free(RRLog *rrlog, RRLogEntry *entry)
{
    __sync_add_and_fetch(&rrlog->nextEvent, 1);
    rrlog->threads[entry->threadId].usedOff += 1;
}

#endif /* __RRPLAY_H__ */

