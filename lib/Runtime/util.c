
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>
#include <threads.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "util.h"

#define XXH_PRIVATE_API
#include "xxhash.h"

Mutex lockTable[LOCKTABLE_SIZE];

static struct {
    uint32_t	evtid;
    const char*	str;
} eventTable[] =
{
#define RREVENT(_a, _b) { RREVENT_##_a, #_a },
    RREVENT_TABLE
#undef RREVENT
    { 0, 0 }
};

static const char *
Lookup(uint32_t event)
{
    int i;

    for (i = 0; eventTable[i].str != 0; i++) {
	if (eventTable[i].evtid == event) {
	    return eventTable[i].str;
	}
    }

    return "UNKNOWN";
}

void
AssertEvent(RRLogEntry *e, uint32_t evt)
{
    if (e->event != evt) {
	rrMode = RRMODE_NORMAL;
	SYSERROR("Event Logging Divergence!");
	SYSERROR("Expected %s(%08x), Encountered %s(%08x)",
			Lookup(evt), evt, Lookup(e->event), e->event);
	SYSERROR("Event #%lu, Thread #%d", e->eventId, e->threadId);
	SYSERROR("NextEvent #%lu, LastEvent #%lu",
			rrlog->nextEvent, rrlog->lastEvent);
	PANIC();
    }
}

void
AssertReplay(RRLogEntry *e, bool test)
{
    if (!test) {
	rrMode = RRMODE_NORMAL;
	SYSERROR("Event Assertion Divergence!"); 
	SYSERROR("Encountered %s(%08x)", Lookup(e->event), e->event);
	SYSERROR("Event #%lu, Thread #%d", e->eventId, e->threadId);
	SYSERROR("NextEvent #%lu, LastEvent #%lu",
			rrlog->nextEvent, rrlog->lastEvent);
	PANIC();
    }
}

void
AssertOutput(RRLogEntry *e, uint64_t hash, uint8_t *buf, size_t nbytes)
{
    if (hash != hashData(buf, nbytes)) {
	rrMode = RRMODE_NORMAL;
	SYSERROR("Output Divergence Detected!");
	SYSERROR("Encountered %s(%08x)", Lookup(e->event), e->event);
	SYSERROR("Event #%lu, Thread #%d", e->eventId, e->threadId);
	SYSERROR("NextEvent #%lu, LastEvent #%lu",
			rrlog->nextEvent, rrlog->lastEvent);
	PANIC();
    }
}

#define RREVENT_DATA_LEN	32
#define RREVENT_DATA_OFFSET	32

void
logData(uint8_t *buf, size_t len)
{
    int32_t i;
    int32_t recs = len / RREVENT_DATA_LEN;
    int32_t rlen = len % RREVENT_DATA_LEN;
    RRLogEntry *e;

    if (buf == NULL) {
	PANIC();
    }

    if (rrMode == RRMODE_RECORD) {
	for (i = 0; i < recs; i++) {
	    e = RRLog_Alloc(rrlog, threadId);
	    e->threadId = threadId;
	    e->event = RREVENT_DATA;
	    uint8_t *dst = ((uint8_t *)e) + RREVENT_DATA_OFFSET;
	    memcpy(dst, buf, RREVENT_DATA_LEN);
	    RRLog_Append(rrlog, e);
	    buf += RREVENT_DATA_LEN;
	}
	if (rlen) {
	    e = RRLog_Alloc(rrlog, threadId);
	    e->threadId = threadId;
	    e->event = RREVENT_DATA;
	    uint8_t *dst = ((uint8_t *)e) + RREVENT_DATA_OFFSET;
	    memcpy(dst, buf, rlen);
	    RRLog_Append(rrlog, e);
	}
    } else {
	for (i = 0; i < recs; i++) {
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_DATA);
	    uint8_t *src = ((uint8_t *)e) + RREVENT_DATA_OFFSET;
	    memcpy(buf, src, RREVENT_DATA_LEN);
	    RRPlay_Free(rrlog, e);
	    buf += RREVENT_DATA_LEN;
	}
	if (rlen) {
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_DATA);
	    uint8_t *src = ((uint8_t *)e) + RREVENT_DATA_OFFSET;
	    memcpy(buf, src, rlen);
	    RRPlay_Free(rrlog, e);
	}
    }
}

uint64_t
hashData(uint8_t *buf, size_t len)
{
    return XXH64(buf, len, 0);
}

