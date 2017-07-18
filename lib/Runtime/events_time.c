
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include <threads.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <dlfcn.h>

#include <sys/cdefs.h>
#include <sys/syscall.h>

#include <libc_private.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "util.h"

static int (*original_gettimeofday)(struct timeval *tp, struct timezone *tzp) = NULL;
static int (*original_clock_gettime)(clockid_t clock_id, const struct timespec *tp) = NULL;

void Time_Init()
{
    original_gettimeofday =
	(int (*)(struct timeval *, struct timezone *)) dlfunc(RTLD_NEXT, "gettimeofday");
    if (original_gettimeofday == NULL) {
	PERROR(strerror(errno));
    }
    original_clock_gettime =
    (int (*)(clockid_t clock_id, const struct timespec *tp)) dlfunc(RTLD_NEXT, "clock_gettime");
    if (original_clock_gettime == NULL) {
	PERROR(strerror(errno));
    }
}

int
__rr_gettimeofday(struct timeval *tp, struct timezone *tzp)
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return original_gettimeofday(tp, tzp);
    }

    if (rrMode == RRMODE_RECORD) {
	result =  original_gettimeofday(tp, tzp);

	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_GETTIMEOFDAY;
	e->value[0] = (uint64_t)result;

	if (result == -1) {
	    e->value[1] = (uint64_t)errno;
	} else {
	    if (tp != NULL) {
		e->value[1] = (uint64_t)tp->tv_sec;
		e->value[2] = (uint64_t)tp->tv_usec;
	    }
	    if (tzp != NULL) {
		e->value[3] = (uint64_t)tzp->tz_minuteswest;
		e->value[4] = (uint64_t)tzp->tz_dsttime;
	    }
	}
	RRLog_Append(rrlog, e);
    } else {
	e = RRPlay_Dequeue(rrlog, threadId);
	AssertEvent(e, RREVENT_GETTIMEOFDAY);
	result = (int)e->value[0];
	if (result == -1) {
	    errno = (int)e->value[1];
	} else {
	    if (tp != NULL) {
		tp->tv_sec = (time_t)e->value[1];
		tp->tv_usec = (suseconds_t)e->value[2];
	    }
	    if (tzp != NULL) {
		tzp->tz_minuteswest = (int)e->value[3];
		tzp->tz_dsttime = (int)e->value[4];
	    }
	}
	RRPlay_Free(rrlog, e);
    }

    return result;
}

int
__rr_clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return original_clock_gettime(clock_id, tp);
    }

    if (rrMode == RRMODE_RECORD) {
	result = original_clock_gettime(clock_id, tp);

	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_GETTIME;
	e->objectId = (uint64_t)clock_id;
	e->value[0] = (uint64_t)result;

	if (result == -1) {
	    e->value[1] = (uint64_t)errno;
	} else {
	    e->value[2] = (uint64_t)tp->tv_sec;
	    e->value[3] = (uint64_t)tp->tv_nsec;
	}
	RRLog_Append(rrlog, e);
    } else {
	e = RRPlay_Dequeue(rrlog, threadId);
	AssertEvent(e, RREVENT_GETTIME);
	result = (int)e->value[0];
	if (result == -1) {
	    errno = (int)e->value[1];
	} else {
	    tp->tv_sec = (time_t)e->value[2];
	    tp->tv_nsec = (long)e->value[3];
	}
	RRPlay_Free(rrlog, e);
    }

    return result;
}

BIND_REF(clock_gettime);
BIND_REF(gettimeofday);

