
#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <castor/mtx.h>


#define LOCKTABLE_SIZE 4096
#define GETLOCK(_obj) &lockTable[(_obj) % LOCKTABLE_SIZE]

extern Mutex lockTable[LOCKTABLE_SIZE];

static inline void
RRLog_LEnter(uint32_t threadId, uint64_t objId)
{
    RRLogEntry *e = RRLog_Alloc(rrlog, threadId);
    e->event = RREVENT_LOCKED_EVENT;
    e->threadId = threadId;
    e->objectId = objId;
    Mutex_Lock(GETLOCK(objId));
    RRLog_Append(rrlog, e);
}

static inline RRLogEntry *
RRLog_LAlloc(uint32_t threadId)
{
    return RRLog_Alloc(rrlog, threadId);
}

static inline void
RRLog_LAppend(RRLogEntry *entry)
{
    uint64_t objId = entry->objectId;

    RRLog_Append(rrlog, entry);
    Mutex_Unlock(GETLOCK(objId));
}

static inline void
RRPlay_LEnter(uint32_t threadId, uint64_t objId)
{
    RRLogEntry *e = RRPlay_Dequeue(rrlog, threadId);
    AssertEvent(e, RREVENT_LOCKED_EVENT);
    Mutex_Lock(GETLOCK(objId));
    RRPlay_Free(rrlog, e);
}

static inline RRLogEntry *
RRPlay_LDequeue(uint32_t threadId)
{
    return RRPlay_Dequeue(rrlog, threadId);
}

static inline void
RRPlay_LFree(RRLogEntry *entry)
{
    uint64_t objId = entry->objectId;

    RRPlay_Free(rrlog, entry);
    Mutex_Unlock(GETLOCK(objId));
}

#endif /* __EVENTS_H__ */

