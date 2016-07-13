
#ifndef __RUNTIME_H__
#define __RUNTIME_H__

#include <threads.h>

enum RRMODE {
    RRMODE_NORMAL, // Normal
    RRMODE_RECORD, // Debug Recording
    RRMODE_REPLAY, // Replay
    //RRMODE_FDREPLAY, // Replay (Real Files)
    //RRMODE_FTREPLAY, // Replay (Fault Tolerance)
};

extern enum RRMODE rrMode;
extern RRLog *rrlog;
extern thread_local int threadId;

void LogDone();

void RRRecordI(uint32_t eventNum, int i);
void RRReplayI(uint32_t eventNum, int *i);

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

#endif /* __RUNTIME_H__ */

