
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <unistd.h>
#include <pthread.h>

#include <rrlog.h>
#include <rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/rrevent.h>
#include <castor/runtime.h>

#include "events.h"

uint64_t
__castor_rdtsc()
{
    uint64_t val;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    val = __builtin_readcyclecounter();
	    break;
	case RRMODE_RECORD:
	    val = __builtin_readcyclecounter();
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_RDTSC;
	    e->objectId = 0;
	    e->threadId = threadId;
	    e->value[0] = val;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_RDTSC);
	    val = e->value[0];
	    RRPlay_Free(rrlog, e);
	    break;
    }
    return val;
}

void
__castor_load_begin(void *ptr)
{
    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    RRLog_LEnter(threadId, (uint64_t)ptr);
	    break;
	case RRMODE_REPLAY:
	    RRPlay_LEnter(threadId, (uint64_t)ptr);
	    break;
    }
    return;
}

void
__castor_load_end(void *ptr)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_LAlloc(threadId);
	    e->event = RREVENT_ATOMIC_LOAD;
	    e->objectId = (uint64_t)ptr;
	    e->threadId = threadId;
	    RRLog_LAppend(e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_LDequeue(threadId);
	    AssertEvent(e, RREVENT_ATOMIC_LOAD);
	    RRPlay_LFree(e);
	    break;
    }
    return;
}

void
__castor_store_begin(void *ptr)
{
    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    RRLog_LEnter(threadId, (uint64_t)ptr);
	    break;
	case RRMODE_REPLAY:
	    RRPlay_LEnter(threadId, (uint64_t)ptr);
	    break;
    }
    return;
}

void
__castor_store_end(void *ptr)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_LAlloc(threadId);
	    e->event = RREVENT_ATOMIC_STORE;
	    e->objectId = (uint64_t)ptr;
	    e->threadId = threadId;
	    RRLog_LAppend(e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_LDequeue(threadId);
	    AssertEvent(e, RREVENT_ATOMIC_STORE);
	    RRPlay_LFree(e);
	    break;
    }
    return;
}


