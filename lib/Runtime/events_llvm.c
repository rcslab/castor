
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <stdatomic.h>
#include <threads.h>

#include <unistd.h>
#include <errno.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "util.h"

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
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_RDTSC;
	    e->objectId = 0;
	    e->value[0] = val;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, getThreadId());
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
	    RRLog_LEnter(getThreadId(), (uint64_t)ptr);
	    break;
	case RRMODE_REPLAY:
	    RRPlay_LEnter(getThreadId(), (uint64_t)ptr);
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
	    e = RRLog_LAlloc(getThreadId());
	    e->event = RREVENT_ATOMIC_LOAD;
	    e->objectId = (uint64_t)ptr;
	    RRLog_LAppend(e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_LDequeue(getThreadId());
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
	    RRLog_LEnter(getThreadId(), (uint64_t)ptr);
	    break;
	case RRMODE_REPLAY:
	    RRPlay_LEnter(getThreadId(), (uint64_t)ptr);
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
	    e = RRLog_LAlloc(getThreadId());
	    e->event = RREVENT_ATOMIC_STORE;
	    e->objectId = (uint64_t)ptr;
	    RRLog_LAppend(e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_LDequeue(getThreadId());
	    AssertEvent(e, RREVENT_ATOMIC_STORE);
	    RRPlay_LFree(e);
	    break;
    }
    return;
}

void
__castor_cmpxchg_begin(void *ptr)
{
    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    RRLog_LEnter(getThreadId(), (uint64_t)ptr);
	    break;
	case RRMODE_REPLAY:
	    RRPlay_LEnter(getThreadId(), (uint64_t)ptr);
	    break;
    }
    return;
}

void
__castor_cmpxchg_end(void *ptr)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_LAlloc(getThreadId());
	    e->event = RREVENT_ATOMIC_XCHG;
	    e->objectId = (uint64_t)ptr;
	    RRLog_LAppend(e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_LDequeue(getThreadId());
	    AssertEvent(e, RREVENT_ATOMIC_XCHG);
	    RRPlay_LFree(e);
	    break;
    }
    return;
}

void
__castor_rmw_begin(void *ptr)
{
    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    RRLog_LEnter(getThreadId(), (uint64_t)ptr);
	    break;
	case RRMODE_REPLAY:
	    RRPlay_LEnter(getThreadId(), (uint64_t)ptr);
	    break;
    }
    return;
}

void
__castor_rmw_end(void *ptr)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_LAlloc(getThreadId());
	    e->event = RREVENT_ATOMIC_RMW;
	    e->objectId = (uint64_t)ptr;
	    RRLog_LAppend(e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_LDequeue(getThreadId());
	    AssertEvent(e, RREVENT_ATOMIC_RMW);
	    RRPlay_LFree(e);
	    break;
    }
    return;
}

void
__castor_asm_begin()
{
    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    RRLog_LEnter(getThreadId(), (uint64_t)0);
	    break;
	case RRMODE_REPLAY:
	    RRPlay_LEnter(getThreadId(), (uint64_t)0);
	    break;
    }
    return;
}

void
__castor_asm_end()
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_LAlloc(getThreadId());
	    e->event = RREVENT_INLINE_ASM;
	    e->objectId = (uint64_t)0;
	    RRLog_LAppend(e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_LDequeue(getThreadId());
	    AssertEvent(e, RREVENT_INLINE_ASM);
	    RRPlay_LFree(e);
	    break;
    }
    return;
}

