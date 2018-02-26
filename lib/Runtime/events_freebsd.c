
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
#include <errno.h>
#include <fcntl.h>

#include <sys/cdefs.h>
#include <sys/syscall.h>

// sysctl
#include <sys/sysctl.h>

#include <libc_private.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "util.h"

int __rr_sysctl(const int *name, u_int namelen, void *oldp,
	     size_t *oldlenp, const void *newp, size_t newlen)
{
    ssize_t result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS___sysctl, name, namelen, oldp, oldlenp, newp, newlen);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS___sysctl, name, namelen, oldp, oldlenp, newp, newlen);

	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_SYSCTL;
	e->value[0] = (uint64_t)result;
	if (oldp) {
		e->value[1] = *oldlenp;
	}
	if (result == -1) {
	    e->value[2] = (uint64_t)errno;
	}
	RRLog_Append(rrlog, e);

	if (oldp) {
	    logData(oldp, *oldlenp);
	}
    } else {
	e = RRPlay_Dequeue(rrlog, threadId);
	result = (int)e->value[0];
	if (oldp) {
	    *oldlenp = e->value[1];
	}
	AssertEvent(e, RREVENT_SYSCTL);
	if (result == -1) {
	    errno = (int)e->value[2];
	}
	RRPlay_Free(rrlog, e);

	if (oldp) {
	    logData(oldp, *oldlenp);
	}
    }

    return result;
}

__strong_reference(__rr_sysctl, __sysctl);

