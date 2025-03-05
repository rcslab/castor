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

// stat/fstat
#include <sys/stat.h>

// socket
#include <sys/socket.h>

// ioctl
#include <sys/ioccom.h>

#include <sys/event.h>
#include <sys/time.h>

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

#include "fdinfo.h"
#include "system.h"
#include "util.h"

extern int _gettimeofday(struct timeval *tv, struct timezone *tz);
extern int _clock_gettime(clockid_t, struct timespec *ts);

static inline int
__rr_pipe2(int fildes[2], int flags)
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return __rr_syscall(SYS_pipe2, fildes, flags);
    }

    if (rrMode == RRMODE_RECORD) {
	result =  __rr_syscall(SYS_pipe2, fildes, flags);
	e = RRLog_Alloc(rrlog, getThreadId());
	e->event = RREVENT_PIPE2;
	e->value[0] = (uint64_t)result;

	if (result == -1) {
	    e->value[1] = (uint64_t)errno;
	} else {
	    e->value[2] = (uint64_t)fildes[0];
	    e->value[3] = (uint64_t)fildes[1];
	}

	RRLog_Append(rrlog, e);
    } else {
	e = RRPlay_Dequeue(rrlog, getThreadId());
	AssertEvent(e, RREVENT_PIPE2);
	result = (int)e->value[0];

	if (result == -1) {
	    errno = (int)e->value[1];
	} else {
	    int status;
	    int n_fildes[2];

	    status = __rr_syscall(SYS_pipe2, n_fildes, flags);
	    ASSERT(status != -1);

	    fildes[0] = (int)e->value[2];
	    fildes[1] = (int)e->value[3];

	    if (fildes[1] != n_fildes[1]) {
		status = __rr_syscall(SYS_dup2, n_fildes[1], fildes[1]);
		ASSERT(status != -1);
		__rr_syscall(SYS_close, n_fildes[1]);
	    }
	    if (fildes[0] != n_fildes[0]) {
		status = __rr_syscall(SYS_dup2, n_fildes[0], fildes[0]);
		ASSERT(status != -1);
		__rr_syscall(SYS_close, n_fildes[0]);
	    }
	}

	RRPlay_Free(rrlog, e);
    }
    return result;
}

int
__rr_pipe(int fildes[2])
{
    return __rr_pipe2(fildes, 0);
}

int
__rr_open(const char *path, int flags, ...)

{
    int result;
    int mode;
    va_list ap;

    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return __rr_syscall(SYS_open, path, flags, mode);
	case RRMODE_RECORD:
	    result = __rr_syscall(SYS_open, path, flags, mode);
	    RRRecordI(RREVENT_OPEN, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_OPEN, &result);
	    if (result != -1) {
		FDInfo_OpenFile(result, path, flags, mode);
	    }
	    break;
    }

    return result;
}

int
__rr_openat(int fd, const char *path, int flags, ...)
{
    int result;
    int mode;
    va_list ap;

    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return __rr_syscall(SYS_openat, fd, path, flags, mode);
	case RRMODE_RECORD:
	    result = __rr_syscall(SYS_openat, fd, path, flags, mode);
	    RRRecordOI(RREVENT_OPENAT, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_OPENAT, &result);
	    break;
    }

    return result;
}

//XXX we need to document what invariants we maintain on
//XXX replay for the descriptor space and make sure our model is
//XXX completely implement.
int
__rr_close(int fd)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return __rr_syscall(SYS_close, fd);
	case RRMODE_RECORD:
	    result = __rr_syscall(SYS_close, fd);
	    RRRecordOI(RREVENT_CLOSE, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_CLOSE, &result);
	    FDInfo_Close(fd);
	    break;
    }

    return result;
}

ssize_t
__rr_readv(int fd, const struct iovec *iov, int iovcnt)
{
    ssize_t result;
    switch (rrMode) {
        case RRMODE_NORMAL:
            return __rr_syscall(SYS_readv, fd, iov, iovcnt);
        case RRMODE_RECORD:
            result = __rr_syscall(SYS_readv, fd, iov, iovcnt);
            RRRecordOS(RREVENT_READV, fd, result);
            if (result != -1) {
              logData((uint8_t*)iov, (size_t) iovcnt * sizeof(struct iovec)); 
                for (int i = 0; i < iovcnt; i++) {
                    logData((uint8_t*)iov[i].iov_base, iov[i].iov_len);
	        }
	    }
            break;
        case RRMODE_REPLAY:
            RRReplayOS(RREVENT_READV, fd, &result);
            if (result != -1) {
              logData((uint8_t*)iov, (size_t) iovcnt * sizeof(struct iovec)); 
                for (int i = 0; i < iovcnt; i++) {
                    logData((uint8_t*)iov[i].iov_base, iov[i].iov_len);
	        }
            }
            break;
    }

    return result;
}

ssize_t
__rr_preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
    ssize_t result;
    switch (rrMode) {
        case RRMODE_NORMAL:
            return __rr_syscall(SYS_preadv, fd, iov, iovcnt, offset);
        case RRMODE_RECORD:
            result = __rr_syscall(SYS_preadv, fd, iov, iovcnt, offset);
            RRRecordOS(RREVENT_PREADV, fd, result);
            if (result != -1) {
              logData((uint8_t*)iov, (size_t) iovcnt * sizeof(struct iovec)); 
                for (int i = 0; i < iovcnt; i++) {
                    logData((uint8_t*)iov[i].iov_base, iov[i].iov_len);
	        }
	    }
            break;
        case RRMODE_REPLAY:
            RRReplayOS(RREVENT_PREADV, fd, &result);
            if (result != -1) {
              logData((uint8_t*)iov, (size_t) iovcnt * sizeof(struct iovec)); 
                for (int i = 0; i < iovcnt; i++) {
                    logData((uint8_t*)iov[i].iov_base, iov[i].iov_len);
	        }
            }
            break;
    }

    return result;
}

int
__rr___getcwd(char * buf, size_t size)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return __rr_syscall(SYS___getcwd, buf, size);
	case RRMODE_RECORD:
	    result = __rr_syscall(SYS___getcwd, buf, size);
	    RRRecordI(RREVENT_GETCWD, result);
	    if (result == 0) {
		logData((uint8_t*)buf, size);
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_GETCWD, &result);
	    if (result == 0) {
		logData((uint8_t*)buf, size);
	    }
	    break;
    }

    return result;
}

ssize_t
__rr_write(int fd, const void *buf, size_t nbytes)
{
    ssize_t result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return __rr_syscall(SYS_write, fd, buf, nbytes);
    }

    if (rrMode == RRMODE_RECORD) {
	result = __rr_syscall(SYS_write, fd, buf, nbytes);

	e = RRLog_Alloc(rrlog, getThreadId());
	e->event = RREVENT_WRITE;
	e->objectId = (uint64_t)fd;
	e->value[0] = (uint64_t)result;
	if (result != -1) {
	    e->value[1] = hashData((uint8_t *)buf, nbytes);
	} else {
	    e->value[2] = (uint64_t)errno;
	}
	RRLog_Append(rrlog, e);
    } else {
	/*
	 * We should only write the same number of bytes as we did during 
	 * recording.  The output divergence test is more conservative than it 
	 * needs to be.  We can assert only on bytes actually written out.
	 */
	if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
	    // Print console output
	    __rr_syscall(SYS_write, fd, buf, nbytes);
	}

	e = RRPlay_Dequeue(rrlog, getThreadId());
	AssertEvent(e, RREVENT_WRITE);
	result = (ssize_t)e->value[0];
	if (result != -1) {
	    AssertOutput(e, e->value[1], (uint8_t *)buf, nbytes);
	} else {
	    errno = (int)e->value[2];
	}
	RRPlay_Free(rrlog, e);
    }

    return result;
}

ssize_t
__rr_writev(int fd, const struct iovec *iov, int iovcnt)
{
    ssize_t result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return __rr_syscall(SYS_writev, fd, iov, iovcnt);
    }

    /* TODO: hash the actual data to survive the aslr */
    if (rrMode == RRMODE_RECORD) {
	result = __rr_syscall(SYS_writev, fd, iov, iovcnt);

	e = RRLog_Alloc(rrlog, getThreadId());
	e->event = RREVENT_WRITEV;
	e->objectId = (uint64_t)fd;
	e->value[0] = (uint64_t)result;
	if (result != -1) {
          e->value[1] = hashData((uint8_t *)iov, (size_t) iovcnt * sizeof(struct iovec));
	} else {
	    e->value[2] = (uint64_t)errno;
	}
	RRLog_Append(rrlog, e);
    } else {
	/*
	 * We should only write the same number of bytes as we did during 
	 * recording.  The output divergence test is more conservative than it 
	 * needs to be.  We can assert only on bytes actually written out.
	 */
	if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
	    // Print console output
	    __rr_syscall(SYS_writev, fd, iov, iovcnt);
	}

	e = RRPlay_Dequeue(rrlog, getThreadId());
	AssertEvent(e, RREVENT_WRITEV);
	result = (ssize_t)e->value[0];
	if (result != -1) {
      AssertOutput(e, e->value[1], (uint8_t *)iov, (size_t) iovcnt * sizeof(struct iovec));
	} else {
	    errno = (int)e->value[2];
	}
	RRPlay_Free(rrlog, e);
    }

    return result;
}

ssize_t
__rr_pwrite(int fd, const void *buf, size_t nbytes, off_t offset)
{
    ssize_t result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return __rr_syscall(SYS_pwrite, fd, buf, nbytes, offset);
    }

    if (rrMode == RRMODE_RECORD) {
	result = __rr_syscall(SYS_pwrite, fd, buf, nbytes, offset);

	e = RRLog_Alloc(rrlog, getThreadId());
	e->event = RREVENT_PWRITE;
	e->objectId = (uint64_t)fd;
	e->value[0] = (uint64_t)result;
	if (result == -1) {
	    e->value[1] = (uint64_t)errno;
	} else {
	    e->value[2] = hashData((uint8_t *)buf, nbytes);
	}
	RRLog_Append(rrlog, e);
    } else {
	/*
	 * We should only write the same number of bytes as we did during 
	 * recording.  The output divergence test is more conservative than it 
	 * needs to be.  We can assert only on bytes actually written out.
	 */
	if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
	    // Print console output
	    __rr_syscall(SYS_write, fd, buf, nbytes);
	}

	e = RRPlay_Dequeue(rrlog, getThreadId());
	AssertEvent(e, RREVENT_PWRITE);
	result = (ssize_t)e->value[0];
	if (result == -1) {
	    errno = (int)e->value[1];
	} else {
	    AssertOutput(e, e->value[2], (uint8_t *)buf, nbytes);
	}
	RRPlay_Free(rrlog, e);
    }

    return result;
}

int
__rr_ioctl(int fd, unsigned long request, ...)
{
    int result;
    va_list ap;
    char *argp;
    bool output = ((IOC_OUT & request) == IOC_OUT);
    unsigned long datalen = IOCPARM_LEN(request);

    va_start(ap, request);
    argp = va_arg(ap, char *);
    va_end(ap);

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return __rr_syscall(SYS_ioctl, fd, request, argp);
	case RRMODE_RECORD:
	    result = __rr_syscall(SYS_ioctl, fd, request, argp);
	    RRRecordOI(RREVENT_IOCTL, fd, result);
	    if ((result == 0) && output) {
		logData((uint8_t *)argp, (size_t)datalen);
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_IOCTL, &result);
	    if ((result == 0) && output) {
		logData((uint8_t *)argp, (size_t)datalen);
	    }
	    break;
    }

    return result;
}

int
__rr_fcntl(int fd, int cmd, ...)
{
    int result;
    va_list ap;
    int64_t arg;

    va_start(ap, cmd);
    arg = va_arg(ap, int64_t);
    va_end(ap);

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return __rr_syscall(SYS_fcntl, fd, cmd, arg);
	case RRMODE_RECORD:
	// XXX: a big switch statement should take care of separate flags first
	// categorize flags into normal case or other cases
	// then have the record/replay switch inside.
	    if (cmd == F_GETLK) {
		LOG("CHECK: upper app is calling F_GETLK\n");
	    }
	    if (cmd == F_SETLK) {
		LOG("CHECK: upper app is calling F_SETLK\n");
	    }
	    ASSERT_IMPLEMENTED((cmd == F_GETFL)  || (cmd == F_SETFL) ||
			               (cmd == F_GETFD)  || (cmd == F_SETFD) ||
			               (cmd == F_DUPFD)  || (cmd == F_DUPFD_CLOEXEC) ||
			               (cmd == F_DUP2FD) || (cmd == F_DUP2FD_CLOEXEC) ||
			               (cmd == F_GETOWN) || (cmd == F_SETOWN) ||
			               (cmd == F_RDAHEAD)|| (cmd == F_READAHEAD) ||
	    //XXX: neither of these are correctly implemented yet, but probably won't break often.
			               (cmd == F_GETLK)  || (cmd == F_SETLK) ||
				       (cmd == F_ISUNIONSTACK));
	    result = __rr_syscall(SYS_fcntl, fd, cmd, arg);
	    RRRecordOI(RREVENT_FCNTL, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_FCNTL, &result);
	    break;
    }

    return result;
}


void log_msg(struct msghdr *msg)
{
    logData((uint8_t*)msg, sizeof(*msg));

    if ((msg->msg_name != NULL) && (msg->msg_namelen > 0)) {
	logData((uint8_t*)msg->msg_name, msg->msg_namelen);
    }

    if ((msg->msg_iov != NULL) && (msg->msg_iovlen > 0)) {
	logData((uint8_t*)msg->msg_iov, (uint64_t)msg->msg_iovlen * sizeof(*msg->msg_iov));
	for (int i = 0; i < msg->msg_iovlen; i++) {
	    logData(msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len);
	}
    }
    if ((msg->msg_control != NULL) && (msg->msg_controllen > 0)) {
	logData((uint8_t*)msg->msg_control, msg->msg_controllen);
    }
}

ssize_t
__rr_recvmsg(int s, struct msghdr *msg, int flags)
{
    ssize_t result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return __rr_syscall(SYS_recvmsg, s, msg, flags);
	case RRMODE_RECORD:
	    result = __rr_syscall(SYS_recvmsg, s, msg, flags);
	    RRRecordOS(RREVENT_RECVMSG, s, result);
	    if (result != -1) {
		log_msg(msg);
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayOS(RREVENT_RECVMSG, s, &result);
	    if (result != -1) {
		log_msg(msg);
	    }
	    break;
    }

    return result;
}


ssize_t
__rr_recvfrom(int s, void *buf, size_t len, int flags, struct sockaddr *
	restrict from, socklen_t * restrict fromlen)
{

    ssize_t result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return __rr_syscall(SYS_recvfrom, s, buf, len, flags, from, fromlen);
	case RRMODE_RECORD:
	    result = __rr_syscall(SYS_recvfrom, s, buf, len, flags, from, fromlen);
	    RRRecordOS(RREVENT_RECVFROM, s, result);
	    if (result != -1) {
		logData((uint8_t*)buf, len);
		if (from != NULL) {
		    logData((uint8_t*)from, sizeof(struct sockaddr));
		    logData((uint8_t*)&fromlen, sizeof(socklen_t));
		}
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayOS(RREVENT_RECVFROM, s, &result);
	    if (result != -1) {
		logData((uint8_t*)buf, len);
		if (from != NULL) {
		    logData((uint8_t*)from, sizeof(struct sockaddr));
		    logData((uint8_t*)&fromlen, sizeof(socklen_t));
		}
	    }
	    break;
    }

    return result;
}

int
__rr_gettimeofday(struct timeval *tp, struct timezone *tzp)
{
    int result;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return _gettimeofday(tp, tzp);
	case RRMODE_RECORD:
	    result = _gettimeofday(tp, tzp);

	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_GETTIMEOFDAY;
	    e->value[0] = (uint64_t)result;
	    if (result == -1) {
		e->value[1] = (uint64_t) errno;
	    } else {
		e->value[2] = (uint64_t)(tp->tv_sec);
		e->value[3] = (uint64_t)(tp->tv_usec);
	    }
	    RRLog_Append(rrlog, e);

	    if (result != -1 && tzp) {
		logData((uint8_t *)tzp, sizeof(struct timezone));
	    }
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_GETTIMEOFDAY);
	    result = (int)e->value[0];
	    if (result == -1) {
		errno = e->value[1];
	    } else {
		tp->tv_sec = e->value[2];
		tp->tv_usec = e->value[3];
	    }
	    RRPlay_Free(rrlog, e);

	    if (result != -1 && tzp) {
		logData((uint8_t *)tzp, sizeof(struct timezone));
	    }
	    break;
    }

    return result;
}

int
__rr_clock_gettime(clockid_t id, struct timespec *tp)
{
    int result;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return _clock_gettime(id, tp);
	case RRMODE_RECORD:
	    result = _clock_gettime(id, tp);

	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_CLOCK_GETTIME;
	    e->objectId = id;
	    e->value[0] = (uint64_t)result;
	    if (result == -1) {
		e->value[1] = (uint64_t) errno;
	    } else {
		e->value[2] = (uint64_t) tp->tv_sec;
		e->value[3] = (uint64_t) tp->tv_nsec;
	    }
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_CLOCK_GETTIME);
	    result = (int)e->value[0];
	    if (result == -1) {
		errno = e->value[1];
	    } else {
		tp->tv_sec = (time_t)e->value[2];
		tp->tv_nsec = e->value[3];
	    }
	    RRPlay_Free(rrlog, e);
	    break;
    }

    return result;
}

int __rr_fstatat(int fd, const char *path, struct stat *buf, int flag);

int
__rr_stat(const char *path, struct stat *ub)
{
    return __rr_fstatat(AT_FDCWD, path, ub, 0);
}

int
__rr_lstat(const char *path, struct stat *ub)
{
    return __rr_fstatat(AT_FDCWD, path, ub, AT_SYMLINK_NOFOLLOW);
}

ssize_t __rr_getdirentries(int fd, char *buf, size_t count, off_t *basep);

ssize_t
__rr_getdents(int fd, char *buf, size_t count)
{
    return __rr_getdirentries(fd, buf, count, NULL);
}


BIND_REF(write);
BIND_REF(close);
BIND_REF(fcntl);
BIND_REF(__getcwd); //do this funny trick with name to avoid conflicting
		    //with libc definition.
BIND_REF(openat);
BIND_REF(recvmsg);
BIND_REF(recvfrom);
BIND_REF(open);
BIND_REF(ioctl);
BIND_REF(pipe);
BIND_REF(pipe2);
BIND_REF(pwrite);
BIND_REF(readv);
BIND_REF(preadv);
BIND_REF(writev);
BIND_REF(stat);
BIND_REF(lstat);
BIND_REF(getdents);

__strong_reference(__rr_gettimeofday, gettimeofday);
__strong_reference(__rr_clock_gettime, clock_gettime);

//XXX: This is ugly, for some reason things sometimes break strangely when this lives in
//its own object fil.

#include "events_gen.c"
