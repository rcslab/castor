
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

#include <stdatomic.h>
#include <threads.h>

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/cdefs.h>
#include <sys/syscall.h>

// wait/waitpid
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <libc_private.h>

#include <castor/debug.h>
#include <castor/rrshared.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "system.h"
#include "util.h"

pid_t
__rr_fork(void)
{
    pid_t result;
    uintptr_t thrNo;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return __rr_syscall(SYS_fork);
    }

    if (rrMode == RRMODE_RECORD) {
	thrNo = RRShared_AllocThread(rrlog);

	e = RRLog_Alloc(rrlog, getThreadId());
	e->event = RREVENT_FORK;
	e->value[1] = thrNo;
	RRLog_Append(rrlog, e);

	result = __rr_syscall(SYS_fork);

	if (result == 0) {
	    setThreadId(thrNo);
	} else {
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_FORKEND;
	    e->value[0] = (uint64_t)result;
	    if (result == -1) {
		e->value[1] = (uint64_t)errno;
	    }
	    RRLog_Append(rrlog, e);
	}
    } else {
	e = RRPlay_Dequeue(rrlog, getThreadId());
	AssertEvent(e, RREVENT_FORK);
	thrNo = e->value[1];
	RRPlay_Free(rrlog, e);

	RRShared_SetupThread(rrlog, thrNo);

	int rstatus = __rr_syscall(SYS_fork);
	if (rstatus < 0) {
	    PERROR("Unable to fork on replay!");
	}

	if (rstatus != 0) {
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_FORKEND);
	    result = (int)e->value[0];
	    if (result == -1) {
		errno = (int)e->value[1];
	    }
	    RRPlay_Free(rrlog, e);
	} else {
	    setThreadId(thrNo);
	    result = 0;
	}
    }

    return result;
}

void
__rr_exit(int status)
{
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	__rr_syscall(SYS_exit, status);
	__builtin_unreachable();
    }

    if (rrMode == RRMODE_RECORD) {
	e = RRLog_Alloc(rrlog, getThreadId());
	e->event = RREVENT_EXIT;
	RRLog_Append(rrlog, e);
    } else {
	e = RRPlay_Dequeue(rrlog, getThreadId());
	AssertEvent(e, RREVENT_EXIT);
	RRPlay_Free(rrlog, e);
    }

    __rr_syscall(SYS_exit, status);
}

/*
 * Will remove this function and replace is it with wait4/6 through the
 * interposing table once pidmap is checked in.
 */
pid_t
__rr_wait(int *status)
{
    pid_t pid;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return __rr_syscall(SYS_wait4, WAIT_ANY, status, 0, NULL);
    }

    if (rrMode == RRMODE_RECORD) {
	pid = __rr_syscall(SYS_wait4, WAIT_ANY, status, 0, NULL);
	e = RRLog_Alloc(rrlog, getThreadId());
	e->event = RREVENT_WAIT;
	e->value[0] = (uint64_t)pid;
	if (pid == -1) {
	    e->value[1] = (uint64_t)errno;
	}
	if (status != NULL) {
	    e->value[2] = (uint64_t)*status;
	}
	RRLog_Append(rrlog, e);
    } else {
	// XXX: Use waitpid to wait for the correct child
	__rr_syscall(SYS_wait4, WAIT_ANY, NULL, 0, NULL);
	e = RRPlay_Dequeue(rrlog, getThreadId());
	AssertEvent(e, RREVENT_WAIT);
	pid = (int)e->value[0];
	if (pid == -1) {
	    errno = (int)e->value[1];
	}
	if (status != NULL) {
	    *status = (int)e->value[2];
	}
	RRPlay_Free(rrlog, e);
    }

    return pid;
}

BIND_REF(exit);
BIND_REF(wait);
BIND_REF(fork);

