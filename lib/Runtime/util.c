
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <threads.h>

#include <castor/archconfig.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "util.h"

Mutex lockTable[LOCKTABLE_SIZE];

void
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

void
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

#define RREVENT_DATA_LEN	32
#define RREVENT_DATA_OFFSET	32

void
logData(uint8_t *buf, size_t len)
{
    int32_t i;
    int32_t recs = len / RREVENT_DATA_LEN;
    int32_t rlen = len % RREVENT_DATA_LEN;
    RRLogEntry *e;

    if (rrMode == RRMODE_RECORD) {
	for (i = 0; i < recs; i++) {
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_DATA;
	    uint8_t *dst = ((uint8_t *)e) + RREVENT_DATA_OFFSET;
	    memcpy(dst, buf, RREVENT_DATA_LEN);
	    RRLog_Append(rrlog, e);
	    buf += RREVENT_DATA_LEN;
	}
	if (rlen) {
	    e = RRLog_Alloc(rrlog, threadId);
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

