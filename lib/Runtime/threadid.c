
#include <stdint.h>
#include <threads.h>
#include <sys/syscall.h>

#include <castor/rrshared.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/events.h>

#include "util.h"
#include "system.h"

static thread_local uint64_t thrId;

uint64_t
getThreadId()
{
	return thrId;
}

uint64_t 
getRecordedPid()
{
	return rrlog->threadInfo[thrId].recordedPid;
}

void 
setRecordedPid(uint64_t p)
{
	ASSERT(rrlog->threadInfo[thrId].recordedPid == -1);
	rrlog->threadInfo[thrId].recordedPid = p;
}

void
setThreadId(uint64_t t, uint64_t p)
{
	long tid;

	__rr_syscall(SYS_thr_self, &tid);

	thrId = t;
	rrlog->threadInfo[thrId].pid = __rr_syscall(SYS_getpid);
	rrlog->threadInfo[thrId].tid = tid;
	ASSERT((rrMode != RRMODE_RECORD) || (p == 0));
	rrlog->threadInfo[thrId].recordedPid = p;
}


