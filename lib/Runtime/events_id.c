
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

BIND_REF(getgroups);
