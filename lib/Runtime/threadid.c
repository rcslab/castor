
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

void
setThreadId(uint64_t t)
{
	long tid;

	__rr_syscall(SYS_thr_self, &tid);

	thrId = t;
	rrlog->threadInfo[thrId].pid = __rr_syscall(SYS_getpid);
	rrlog->threadInfo[thrId].tid = tid;
}


