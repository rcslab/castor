
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

// poll/kqueue
#include <poll.h>
#include <sys/event.h>
#include <sys/time.h>

// getdirentries
#include <sys/types.h>
#include <dirent.h>

// statfs/fstatfs
#include <sys/param.h>
#include <sys/mount.h>

#include <libc_private.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "util.h"

int
__rr_getdirentries(int fd, char *buf, int nbytes, long *basep)
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS_getdirentries, fd, buf, nbytes, basep);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS_getdirentries, fd, buf, nbytes, basep);
	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_GETDIRENTRIES;
	e->objectId = (uint64_t)fd;
	e->value[0] = (uint64_t)result;
	e->value[1] = (uint64_t)*basep;
	if (result == -1){
	    e->value[2] = (uint64_t)errno;
	}
	RRLog_Append(rrlog, e);
	if (result >= 0) {
	    logData((uint8_t*)buf, (size_t)result);
	}
    } else {
	e = RRPlay_Dequeue(rrlog, threadId);
	AssertEvent(e, RREVENT_GETDIRENTRIES);
	result = (int)e->value[0];
	*basep = (long)e->value[1];
	if (result == -1) {
	    errno = (int)e->value[2];
	}
	RRPlay_Free(rrlog, e);

	if (result >= 0) {
	    logData((uint8_t*)buf, (size_t)result);
	}
    }

    return result;
}

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

int
__rr_kqueue()
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_kqueue);
	case RRMODE_RECORD:
	    result = syscall(SYS_kqueue);
	    RRRecordI(RREVENT_KQUEUE, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_KQUEUE, &result);
	    break;
    }

    return result;
}

int
__rr_kevent(int kq, const struct kevent *changelist, int nchanges,
	struct kevent *eventlist, int nevents,
	const struct timespec *timeout)
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS_kevent, kq, changelist, nchanges,
		       eventlist, nevents, timeout);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS_kevent, kq, changelist, nchanges,
			 eventlist, nevents, timeout);
	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_KEVENT;
	e->objectId = (uint64_t)kq;
	e->value[0] = (uint64_t)result;
	e->value[1] = (uint64_t)nchanges;
	e->value[2] = (uint64_t)nevents;
	if (result == -1) {
	    e->value[3] = (uint64_t)errno;
	}
	RRLog_Append(rrlog, e);

	if (result != -1) {
	    if (eventlist != NULL) {
		logData((uint8_t *)eventlist, sizeof(struct kevent) * 
			(uint64_t)result);
	    }
	}
    } else {
	e = RRPlay_Dequeue(rrlog, threadId);
	AssertEvent(e, RREVENT_KEVENT);
	result = (int)e->value[0];
	if (result == -1) {
	    errno = (int)e->value[3];
	}
	RRPlay_Free(rrlog, e);

	if (result != -1) {
	    if (eventlist != NULL) {
		logData((uint8_t *)eventlist, sizeof(struct kevent) * 
			(uint64_t)result);
	    }
	}
    }

    return result;
}


BIND_REF(getdirentries);
//BIND_REF(kqueue);
BIND_REF(kevent);
__strong_reference(__rr_sysctl, __sysctl);

