
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

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "util.h"

int
__rr_getgroups(int gidsetlen, gid_t *gidset)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_getgroups, gidsetlen, gidset);
	case RRMODE_RECORD:
	    result = syscall(SYS_getgroups, gidsetlen, gidset);
	    RRRecordI(RREVENT_GETGROUPS, result);
	    if (result > 0) {
		logData((uint8_t *)gidset, (uint64_t)result * sizeof(gid_t));
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_GETGROUPS, &result);
	    if (result > 0) {
		logData((uint8_t *)gidset, (uint64_t)result * sizeof(gid_t));
	    }
	    break;
    }

    return result;
}

static inline id_t
id_get(int syscallNum, uint32_t eventNum)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(syscallNum);
	case RRMODE_RECORD:
	    result = syscall(syscallNum);
	    RRRecordI(eventNum, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(eventNum, &result);
	    break;
    }

    return result;
}

uid_t
__rr_getuid(void)
{
    return id_get(SYS_getuid, RREVENT_GETUID);
}

uid_t
__rr_geteuid(void)
{
    return id_get(SYS_geteuid, RREVENT_GETEUID);
}

gid_t
__rr_getgid(void)
{
    return id_get(SYS_getgid, RREVENT_GETGID);
}

gid_t
__rr_getegid(void)
{
    return id_get(SYS_getegid, RREVENT_GETEGID);
}

BIND_REF(getgroups);
BIND_REF(getuid);
BIND_REF(geteuid);
BIND_REF(getgid);
BIND_REF(getegid);

