
#ifndef __RUNTIME_H__
#define __RUNTIME_H__

#include <castor/rrlog.h>
#include <castor/rrplay.h>

extern enum RRMODE rrMode;
extern RRLog *rrlog;
extern thread_local int threadId;

#if defined(CASTOR_DEBUG)
static inline void
AssertEvent(RRLogEntry *e, int evt)
{
    if (e->event != evt) {
	rrMode = RRMODE_NORMAL;
	printf("Expected %08x, Encountered %08x\n", evt, e->event);
	printf("Event #%lu, Thread #%d\n", e->eventId, e->threadId);
	printf("NextEvent #%lu, LastEvent #%lu\n", rrlog->nextEvent, rrlog->lastEvent);
	abort();
    }
}

static inline void
AssertReplay(RRLogEntry *e, bool test)
{
    if (!test) {
	rrMode = RRMODE_NORMAL;
	printf("Encountered %08x\n", e->event);
	printf("Event #%lu, Thread #%d\n", e->eventId, e->threadId);
	printf("NextEvent #%lu, LastEvent #%lu\n", rrlog->nextEvent, rrlog->lastEvent);
	abort();
    }
}
#elif defined(CASTOR_RELEASE)
#define AssertEvent(_e, _evt)
#define AssertReplay(_e, _tst)
#else
#error "Must define build type"
#endif

static inline void
RRRecordI(uint32_t eventNum, int i)
{
    RRLogEntry *e;

    e = RRLog_Alloc(rrlog, threadId);
    e->event = eventNum;
    e->threadId = threadId;
    e->objectId = 0;
    e->value[0] = i;
    RRLog_Append(rrlog, e);
}

static inline void
RRReplayI(uint32_t eventNum, int *i)
{
    RRLogEntry *e;

    e = RRPlay_Dequeue(rrlog, threadId);
    AssertEvent(e, eventNum);
    *i = e->value[0];
    RRPlay_Free(rrlog, e);
}

static inline void
RRRecordOI(uint32_t eventNum, int od, int i)
{
    RRLogEntry *e;

    e = RRLog_Alloc(rrlog, threadId);
    e->event = eventNum;
    e->threadId = threadId;
    e->objectId = od;
    e->value[0] = i;
    RRLog_Append(rrlog, e);
}

static inline void
RRReplayOI(uint32_t eventNum, int *od, int *i)
{
    RRLogEntry *e;

    e = RRPlay_Dequeue(rrlog, threadId);
    AssertEvent(e, eventNum);
    if (od != NULL) {
	*od = e->objectId;
    }
    if (i != NULL) {
	*i =  e->value[0];
    }
    RRPlay_Free(rrlog, e);
}

static inline void
RRRecordOU(uint32_t eventNum, int od, uint64_t u)
{
    RRLogEntry *e;

    e = RRLog_Alloc(rrlog, threadId);
    e->event = eventNum;
    e->threadId = threadId;
    e->objectId = od;
    e->value[0] = u;
    RRLog_Append(rrlog, e);
}

static inline void
RRReplayOU(uint32_t eventNum, int *od, uint64_t *u)
{
    RRLogEntry *e;

    e = RRPlay_Dequeue(rrlog, threadId);
    AssertEvent(e, eventNum);
    if (od != NULL) {
	*od = e->objectId;
    }
    if (u != NULL) {
	*u =  e->value[0];
    }
    RRPlay_Free(rrlog, e);
}

#endif /* __RUNTIME_H__ */

