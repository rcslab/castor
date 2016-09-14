
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

