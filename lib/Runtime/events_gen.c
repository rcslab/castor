
/*** this code has been autogenerated by code in utils/gen, do not modify it directly. ***/


#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/stat.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "util.h"




#include <sys/param.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/user.h>

#if !defined(PAD64_REQUIRED) && (defined(__powerpc__) || defined(__mips__))
#define PAD64_REQUIRED
#endif












int
__rr_getrusage(int who, struct rusage * rusage)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_getrusage, who, rusage);
	case RRMODE_RECORD:
	    result = syscall(SYS_getrusage, who, rusage);
	    RRRecordOI(RREVENT_GETRUSAGE, who, result);
		if (result == 0) {
			logData((uint8_t *)rusage, sizeof(struct rusage));
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_GETRUSAGE, who, &result);
		if (result == 0) {
			logData((uint8_t *)rusage, sizeof(struct rusage));
		}
	    break;
    }
    return result;
}

int
__rr_shutdown(int s, int how)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_shutdown, s, how);
	case RRMODE_RECORD:
	    result = syscall(SYS_shutdown, s, how);
	    RRRecordOI(RREVENT_SHUTDOWN, s, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_SHUTDOWN, s, &result);
	    break;
    }
    return result;
}

int
__rr_getrlimit(int which, struct rlimit * rlp)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_getrlimit, which, rlp);
	case RRMODE_RECORD:
	    result = syscall(SYS_getrlimit, which, rlp);
	    RRRecordOI(RREVENT_GETRLIMIT, which, result);
		if (result == 0) {
			logData((uint8_t *)rlp, sizeof(struct rlimit));
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_GETRLIMIT, which, &result);
		if (result == 0) {
			logData((uint8_t *)rlp, sizeof(struct rlimit));
		}
	    break;
    }
    return result;
}

int
__rr_setrlimit(int which, const struct rlimit * rlp)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_setrlimit, which, rlp);
	case RRMODE_RECORD:
	    result = syscall(SYS_setrlimit, which, rlp);
	    RRRecordOI(RREVENT_SETRLIMIT, which, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_SETRLIMIT, which, &result);
	    break;
    }
    return result;
}

int
__rr_cpuset(cpusetid_t * setid)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_cpuset, setid);
	case RRMODE_RECORD:
	    result = syscall(SYS_cpuset, setid);
	    RRRecordI(RREVENT_CPUSET, result);
		if (result == 0) {
			logData((uint8_t *)setid, sizeof(cpusetid_t));
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_CPUSET, &result);
		if (result == 0) {
			logData((uint8_t *)setid, sizeof(cpusetid_t));
		}
	    break;
    }
    return result;
}

int
__rr_cpuset_setid(cpuwhich_t which, id_t id, cpusetid_t setid)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_cpuset_setid, which, id, setid);
	case RRMODE_RECORD:
	    result = syscall(SYS_cpuset_setid, which, id, setid);
	    RRRecordI(RREVENT_CPUSET_SETID, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_CPUSET_SETID, &result);
	    break;
    }
    return result;
}

int
__rr_cpuset_getid(cpulevel_t level, cpuwhich_t which, id_t id, cpusetid_t * setid)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_cpuset_getid, level, which, id, setid);
	case RRMODE_RECORD:
	    result = syscall(SYS_cpuset_getid, level, which, id, setid);
	    RRRecordI(RREVENT_CPUSET_GETID, result);
		if (result == 0) {
			logData((uint8_t *)setid, sizeof(cpusetid_t));
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_CPUSET_GETID, &result);
		if (result == 0) {
			logData((uint8_t *)setid, sizeof(cpusetid_t));
		}
	    break;
    }
    return result;
}

int
__rr_cpuset_getaffinity(cpulevel_t level, cpuwhich_t which, id_t id, size_t cpusetsize, cpuset_t * mask)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_cpuset_getaffinity, level, which, id, cpusetsize, mask);
	case RRMODE_RECORD:
	    result = syscall(SYS_cpuset_getaffinity, level, which, id, cpusetsize, mask);
	    RRRecordI(RREVENT_CPUSET_GETAFFINITY, result);
		if (result == 0) {
			logData((uint8_t *)mask, sizeof(cpuset_t));
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_CPUSET_GETAFFINITY, &result);
		if (result == 0) {
			logData((uint8_t *)mask, sizeof(cpuset_t));
		}
	    break;
    }
    return result;
}

int
__rr_cpuset_setaffinity(cpulevel_t level, cpuwhich_t which, id_t id, size_t cpusetsize, const cpuset_t * mask)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_cpuset_setaffinity, level, which, id, cpusetsize, mask);
	case RRMODE_RECORD:
	    result = syscall(SYS_cpuset_setaffinity, level, which, id, cpusetsize, mask);
	    RRRecordI(RREVENT_CPUSET_SETAFFINITY, result);
		if (result == 0) {
			logData((uint8_t *)mask, sizeof(const cpuset_t));
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_CPUSET_SETAFFINITY, &result);
		if (result == 0) {
			logData((uint8_t *)mask, sizeof(const cpuset_t));
		}
	    break;
    }
    return result;
}

int
__rr_fstat(int fd, struct stat * sb)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_fstat, fd, sb);
	case RRMODE_RECORD:
	    result = syscall(SYS_fstat, fd, sb);
	    RRRecordOI(RREVENT_FSTAT, fd, result);
		if (result == 0) {
			logData((uint8_t *)sb, sizeof(struct stat));
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_FSTAT, fd, &result);
		if (result == 0) {
			logData((uint8_t *)sb, sizeof(struct stat));
		}
	    break;
    }
    return result;
}

BIND_REF(getrusage);
BIND_REF(shutdown);
BIND_REF(getrlimit);
BIND_REF(setrlimit);
BIND_REF(cpuset);
BIND_REF(cpuset_setid);
BIND_REF(cpuset_getid);
BIND_REF(cpuset_getaffinity);
BIND_REF(cpuset_setaffinity);
BIND_REF(fstat);
