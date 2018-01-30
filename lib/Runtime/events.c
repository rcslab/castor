
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

// stat/fstat
#include <sys/stat.h>

// sysctl
#include <sys/sysctl.h>

// socket
#include <sys/socket.h>

// ioctl
#include <sys/ioccom.h>

// poll
#include <poll.h>
#include <sys/event.h>
#include <sys/time.h>

// statfs/fstatfs
#include <sys/param.h>
#include <sys/mount.h>

// sendfile
#include <sys/uio.h>

#include <libc_private.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "fdinfo.h"
#include "util.h"

extern void *__sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
extern int __sys_dup2(int oldd, int newd);
extern int __sys_close(int fd);
extern int __sys_pipe(int fildes[2]);

int
__rr_poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS_poll, fds, nfds, timeout);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS_poll, fds, nfds, timeout);
	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_POLL;
	e->value[0] = (uint64_t)result;
	e->value[1] = nfds;
	if (result == -1) {
	    e->value[2] = (uint64_t)errno;
	}
	RRLog_Append(rrlog, e);
	if (result > 0) {
	    logData((uint8_t *)fds, nfds * sizeof(struct pollfd));
	}
    } else {
	e = RRPlay_Dequeue(rrlog, threadId);
	AssertEvent(e, RREVENT_POLL);
	result = (int)e->value[0];
	assert(e->value[1] == nfds);
	if (result == -1) {
	    errno = e->value[2];
	}
	RRPlay_Free(rrlog, e);

	if (result > 0) {
	    logData((uint8_t *)fds, nfds * sizeof(struct pollfd));
	}
    }

    return result;
}

int
__rr_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS_getsockopt, s, level, optname, optval, optlen);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS_getsockopt, s, level, optname, optval, optlen);
	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_GETSOCKOPT;
	e->value[0] = (uint64_t)result;
	e->value[1] = (uint64_t)optname;
	if (optlen != NULL) {
	    e->value[2] = (uint64_t)*optlen;
	}
	if (result == -1) {
	    e->value[3] = (uint64_t)errno;
	}
	RRLog_Append(rrlog, e);

	if (result == 0) {
	    ASSERT(optval != NULL);
	    ASSERT(optlen != NULL);
	    logData((uint8_t *)optval, *optlen);
	}
    } else {
	e = RRPlay_Dequeue(rrlog, threadId);
	AssertEvent(e, RREVENT_GETSOCKOPT);
	result = (int)e->value[0];
	if (optlen != NULL) {
	    *optlen = (socklen_t)e->value[2];
	}
	if (result == -1) {
	    errno = e->value[3];
	}
	RRPlay_Free(rrlog, e);

	if (result == 0) {
	    ASSERT(optval != NULL);
	    ASSERT(optlen != NULL);
	    logData((uint8_t *)optval, *optlen);
	}
    }

    return result;
}

#define SYS_pipe SYS_freebsd10_pipe

static inline int
call_pipe(int callnum, int fildes[2], int flags)
{
    if (callnum == SYS_pipe) {
	return __sys_pipe(fildes);
    } else {
	return syscall(SYS_pipe2, fildes, flags);
    }
}

static inline int
pipe_handler(int callnum, int fildes[2], int flags)
{
    int result;
    RRLogEntry *e;
    uint32_t event_type = (callnum == SYS_pipe) ? RREVENT_PIPE : RREVENT_PIPE2;

    if (rrMode == RRMODE_NORMAL) {
	return call_pipe(callnum, fildes, flags);
    }

    if (rrMode == RRMODE_RECORD) {
	result = call_pipe(callnum, fildes, flags);
	e = RRLog_Alloc(rrlog, threadId);
	e->event = event_type;
	e->value[0] = (uint64_t)result;

	if (result == -1) {
	    e->value[1] = (uint64_t)errno;
	} else {
	    e->value[2] = (uint64_t)fildes[0];
	    e->value[3] = (uint64_t)fildes[1];
	}

	RRLog_Append(rrlog, e);
    } else {
	e = RRPlay_Dequeue(rrlog, threadId);
	AssertEvent(e, event_type);
	result = (int)e->value[0];

	if (result == -1) {
	    errno = (int)e->value[1];
	} else {
	    int status;
	    int n_fildes[2];

	    status = call_pipe(callnum, n_fildes, flags);
	    ASSERT(status != -1);

	    fildes[0] = (int)e->value[2];
	    fildes[1] = (int)e->value[3];

	    if (fildes[1] != n_fildes[1]) {
		status = __sys_dup2(n_fildes[1], fildes[1]);
		ASSERT(status != -1);
		__sys_close(n_fildes[1]);
	    }
	    if (fildes[0] != n_fildes[0]) {
		status = __sys_dup2(n_fildes[0], fildes[0]);
		ASSERT(status != -1);
		__sys_close(n_fildes[0]);
	    }
	}

	RRPlay_Free(rrlog, e);
    }
    return result;
}

int
__rr_pipe(int fildes[2])
{
    return pipe_handler(SYS_pipe, fildes, 0);
}

int
__rr_pipe2(int fildes[2], int flags)
{
    return pipe_handler(SYS_pipe2, fildes, flags);
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
	    return syscall(SYS_open, path, flags, mode);
	case RRMODE_RECORD:
	    result = syscall(SYS_open, path, flags, mode);
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
	    return syscall(SYS_openat, fd, path, flags, mode);
	case RRMODE_RECORD:
	    result = syscall(SYS_openat, fd, path, flags, mode);
	    RRRecordOI(RREVENT_OPENAT, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_OPENAT, &result);
	    break;
    }

    return result;
}

int
__rr_close(int fd)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_close, fd);
	case RRMODE_RECORD:
	    result = syscall(SYS_close, fd);
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
__rr_read(int fd, void *buf, size_t nbytes)
{
    ssize_t result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_read, fd, buf, nbytes);
	case RRMODE_RECORD:
	    result = syscall(SYS_read, fd, buf, nbytes);
	    RRRecordOS(RREVENT_READ, fd, result);
	    if (result != -1) {
		logData((uint8_t*)buf, nbytes);
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayOS(RREVENT_READ, fd, &result);
	    if (result != -1) {
		logData((uint8_t*)buf, nbytes);
	    }
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
            return syscall(SYS_readv, fd, iov, iovcnt);
        case RRMODE_RECORD:
            result = syscall(SYS_readv, fd, iov, iovcnt);
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


int
__rr_getcwd(char * buf, size_t size)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS___getcwd, buf, size);
	case RRMODE_RECORD:
	    result = syscall(SYS___getcwd, buf, size);
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
	return syscall(SYS_write, fd, buf, nbytes);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS_write, fd, buf, nbytes);

	e = RRLog_Alloc(rrlog, threadId);
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
	    syscall(SYS_write, fd, buf, nbytes);
	}

	e = RRPlay_Dequeue(rrlog, threadId);
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
	return syscall(SYS_writev, fd, iov, iovcnt);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS_writev, fd, iov, iovcnt);

	e = RRLog_Alloc(rrlog, threadId);
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
	    syscall(SYS_writev, fd, iov, iovcnt);
	}

	e = RRPlay_Dequeue(rrlog, threadId);
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
	return syscall(SYS_pwrite, fd, buf, nbytes, offset);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS_pwrite, fd, buf, nbytes, offset);

	e = RRLog_Alloc(rrlog, threadId);
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
	    syscall(SYS_write, fd, buf, nbytes);
	}

	e = RRPlay_Dequeue(rrlog, threadId);
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
	    return syscall(SYS_ioctl, fd, request, argp);
	case RRMODE_RECORD:
	    result = syscall(SYS_ioctl, fd, request, argp);
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
	    return syscall(SYS_fcntl, fd, cmd, arg);
	case RRMODE_RECORD:
	// XXX: a big switch statement should take care of separate flags first
	// categorize flags into normal case or other cases
	// then have the record/replay switch inside.
	    ASSERT_IMPLEMENTED((cmd == F_GETFL)  || (cmd == F_SETFL) ||
			               (cmd == F_GETFD)  || (cmd == F_SETFD) ||
			               (cmd == F_DUPFD)  || (cmd == F_DUPFD_CLOEXEC) ||
			               (cmd == F_DUP2FD) || (cmd == F_DUP2FD_CLOEXEC) ||
			               (cmd == F_GETOWN) || (cmd == F_SETOWN) ||
			               (cmd == F_RDAHEAD)|| (cmd == F_READAHEAD) ||
	    //XXX: neither of these are correctly implemented yet, but probably won't break often.
			               (cmd == F_GETLK)  || (cmd == F_SETLK));
	    result = syscall(SYS_fcntl, fd, cmd, arg);
	    RRRecordOI(RREVENT_FCNTL, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_FCNTL, &result);
	    break;
    }

    return result;
}

int
__rr_socket(int domain, int type, int protocol)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_socket, domain, type, protocol);
	case RRMODE_RECORD:
	    result = syscall(SYS_socket, domain, type, protocol);
	    RRRecordI(RREVENT_SOCKET, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SOCKET, &result);
	    break;
    }

    return result;
}

int
__rr_bind(int s, const struct sockaddr *addr, socklen_t addrlen)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_bind, s, addr, addrlen);
	case RRMODE_RECORD:
	    result = syscall(SYS_bind, s, addr, addrlen);
	    RRRecordOI(RREVENT_BIND, s, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_BIND, &result);
	    break;
    }

    return result;
}

int
__rr_listen(int s, int backlog)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_listen, s, backlog);
	case RRMODE_RECORD:
	    result = syscall(SYS_listen, s, backlog);
	    RRRecordOI(RREVENT_LISTEN, s, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_LISTEN, &result);
	    break;
    }

    return result;
}

static inline int
call_accept(int callnum, int s, struct sockaddr * restrict addr,
          socklen_t * restrict addrlen, int flags)
{
    if (callnum == SYS_accept) {
	return syscall(SYS_accept, s, addr, addrlen);
    } else {
	return syscall(SYS_accept4, s, addr, addrlen, flags);
    }
}


static inline int
accept_handler(int callnum, int s, struct sockaddr * restrict addr,
          socklen_t * restrict addrlen, int flags)
{
    int result;
    uint32_t event_type = (callnum == SYS_accept) ? RREVENT_ACCEPT : RREVENT_ACCEPT4;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return call_accept(callnum, s, addr, addrlen, flags);
	case RRMODE_RECORD:
	    result = call_accept(callnum, s, addr, addrlen, flags);
	    RRRecordOI(event_type, s, result);
	    if (result != -1) {
		//XXX: slow, store in value[] directly
		logData((uint8_t*)addrlen, sizeof(*addrlen));
		logData((uint8_t*)addr, *addrlen);
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(event_type, &result);
	    if (result != -1) {
		//XXX: slow, store in value[] directly
		logData((uint8_t*)addrlen, sizeof(*addrlen));
		logData((uint8_t*)addr, *addrlen);
	    }
	    break;
    }

    return result;
}

int
__rr_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    return accept_handler(SYS_accept, s, addr, addrlen, 0);
}

int
__rr_accept4(int s, struct sockaddr *addr, socklen_t *addrlen, int flags)
{

    return accept_handler(SYS_accept4, s, addr, addrlen, flags);
}

ssize_t
__rr_pread(int fd, void *buf, size_t nbytes, off_t offset)
{
    ssize_t result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_pread, fd, buf, nbytes, offset);
	case RRMODE_RECORD:
	    result = syscall(SYS_pread, fd, buf, nbytes, offset);
	    RRRecordOS(RREVENT_PREAD, fd, result);
	    if (result != -1) {
		logData((uint8_t*)buf, nbytes);
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayOS(RREVENT_PREAD, fd, &result);
	    if (result != -1) {
		logData((uint8_t*)buf, nbytes);
	    }
	    break;
    }

    return result;
}

int
__rr_flock(int fd, int operation)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_flock, fd, operation);
	case RRMODE_RECORD:
	    result = syscall(SYS_flock, fd, operation);
	    RRRecordOI(RREVENT_FLOCK, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_FLOCK, &result);
	    break;
    }

    return result;
}

mode_t
__rr_umask(mode_t numask)
{
    uint64_t result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_umask, numask);
	case RRMODE_RECORD:
	    result = (uint64_t)syscall(SYS_umask, numask);
	    RRRecordOU(RREVENT_UMASK, 0, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOU(RREVENT_UMASK, 0, &result);
	    break;
    }

    return (mode_t)result;
}

int __rr_getdents(int fd, char *buf, int nbytes)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_getdents, fd, buf, nbytes);
	case RRMODE_RECORD:
	    result = syscall(SYS_getdents, fd, buf, nbytes);
	    RRRecordOI(RREVENT_GETDENTS, fd, result);
	    if (result > 0) {
		logData((uint8_t*)buf, (size_t)result);
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_GETDENTS, &result);
	    if (result > 0) {
		logData((uint8_t*)buf, (size_t)result);
	    }
	    break;
    }

    return result;
}

int
__rr_getpeername(int s, struct sockaddr * restrict name,
		 socklen_t * restrict namelen)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_getpeername, s, name, namelen);
	case RRMODE_RECORD:
	    result = syscall(SYS_getpeername, s, name, namelen);
	    RRRecordOI(RREVENT_GETPEERNAME, s, result);
	    if (result == 0) {
		//XXX: slow, store in value[] directly
		logData((uint8_t*)namelen, sizeof(*namelen));
		logData((uint8_t*)name, *namelen);
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_GETPEERNAME, &result);
	    if (result == 0) {
		//XXX: slow, store in value[] directly
		logData((uint8_t*)namelen, sizeof(*namelen));
		logData((uint8_t*)name, *namelen);
	    }
	    break;
    }

    return result;
}

int
__rr_getsockname(int s, struct sockaddr * restrict name,
		 socklen_t * restrict namelen)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_getsockname, s, name, namelen);
	case RRMODE_RECORD:
	    result = syscall(SYS_getsockname, s, name, namelen);
	    RRRecordOI(RREVENT_GETSOCKNAME, s, result);
	    if (result == 0) {
		logData((uint8_t*)namelen, sizeof(*namelen));
		logData((uint8_t*)name, *namelen);
		//XXX:slow, store directly in value
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_GETSOCKNAME, &result);
	    if (result == 0) {
		logData((uint8_t*)namelen, sizeof(*namelen));
		logData((uint8_t*)name, *namelen);
		//XXX:slow, store directly in value
	    }
	    break;
    }

    return result;
}

int
__rr_sendfile(int fd, int s, off_t offset, size_t nbytes, struct sf_hdtr *hdtr, off_t *sbytes, int flags)
{
    int result;

    switch (rrMode) {
        case RRMODE_NORMAL:
            return syscall(SYS_sendfile, fd, s, offset, nbytes, hdtr, sbytes, flags);
        case RRMODE_RECORD:
            result = syscall(SYS_sendfile, fd, s, offset, nbytes, hdtr, sbytes, flags);
            RRRecordOI(RREVENT_SENDFILE, s, result);
            break;
        case RRMODE_REPLAY:
            RRReplayOI(RREVENT_SENDFILE, s, &result);
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
	    return syscall(SYS_recvmsg, s, msg, flags);
	case RRMODE_RECORD:
	    result = syscall(SYS_recvmsg, s, msg, flags);
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
        return syscall(SYS_recvfrom, s, buf, len, flags, from, fromlen);
	case RRMODE_RECORD:
	    result = syscall(SYS_recvfrom, s, buf, len, flags, from, fromlen);
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
__rr_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	 struct timeval *timeout)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_select, nfds, readfds, writefds, exceptfds, timeout);
	case RRMODE_RECORD:
	    result = syscall(SYS_select, nfds, readfds, writefds, exceptfds, timeout);
	    RRRecordI(RREVENT_SELECT, result);
	    if (result != -1) {
		if (readfds != NULL) {
		    logData((uint8_t*)readfds, sizeof(*readfds));
		}
		if (writefds != NULL) {
		    logData((uint8_t*)writefds, sizeof(*writefds));
		}
		if (exceptfds != NULL) {
		    logData((uint8_t*)exceptfds, sizeof(*exceptfds));
		}
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SELECT, &result);
	    if (result != -1) {
		if (readfds != NULL) {
		    logData((uint8_t*)readfds, sizeof(*readfds));
		}
		if (writefds != NULL) {
		    logData((uint8_t*)writefds, sizeof(*writefds));
		}
		if (exceptfds != NULL) {
		    logData((uint8_t*)exceptfds, sizeof(*exceptfds));
		}
	    }
	    break;
    }

    return result;
}

int
__rr_dup2(int oldd, int newd)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_dup2, oldd, newd);
	case RRMODE_RECORD:
	    result = syscall(SYS_dup2, oldd, newd);
	    RRRecordOI(RREVENT_DUP2, oldd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_DUP2, oldd, &result);
	    // XXX: FDINFO
	    break;
    }

    return result;
}

int
__rr_dup(int oldd)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_dup, oldd);
	case RRMODE_RECORD:
	    result = syscall(SYS_dup, oldd);
	    RRRecordOI(RREVENT_DUP, oldd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_DUP, oldd, &result);
	    // XXX: FDINFO
	    break;
    }

    return result;
}

void Add_Interposer(int slotNum, interpos_func_t newHandler)
{
    interpos_func_t * slotP = __libc_interposing_slot(slotNum);
    *slotP = (int (*)(void)) newHandler;
}

void
Events_Init()
{
    Add_Interposer(INTERPOS_read, (interpos_func_t)&__rr_read);
    Add_Interposer(INTERPOS_write, (interpos_func_t)&__rr_write);
    Add_Interposer(INTERPOS_openat, (interpos_func_t)&__rr_openat);
    Add_Interposer(INTERPOS_close, (interpos_func_t)&__rr_close);
    Add_Interposer(INTERPOS_fcntl, (interpos_func_t)&__rr_fcntl);
    Add_Interposer(INTERPOS_recvfrom,  (interpos_func_t)&__rr_recvfrom);
    Add_Interposer(INTERPOS_accept,  (interpos_func_t)&__rr_accept);
    Add_Interposer(INTERPOS_poll,  (interpos_func_t)&__rr_poll);
    Add_Interposer(INTERPOS_readv,  (interpos_func_t)&__rr_readv);
    Add_Interposer(INTERPOS_recvmsg,  (interpos_func_t)&__rr_recvmsg);
    Add_Interposer(INTERPOS_select,  (interpos_func_t)&__rr_select);
    Add_Interposer(INTERPOS_writev,  (interpos_func_t)&__rr_writev);
}

__strong_reference(__rr_read, _read);
__strong_reference(__rr_write, _write);
__strong_reference(__rr_close, _close);
__strong_reference(__rr_fcntl, _fcntl);

__strong_reference(__rr_getcwd, __getcwd);

BIND_REF(openat);
BIND_REF(flock);
BIND_REF(umask);
BIND_REF(getpeername);
BIND_REF(getsockname);
BIND_REF(getdents);
BIND_REF(sendfile);
BIND_REF(select);
BIND_REF(recvmsg);
BIND_REF(recvfrom);
BIND_REF(open);
BIND_REF(ioctl);
BIND_REF(socket);
BIND_REF(bind);
BIND_REF(listen);
BIND_REF(accept);
BIND_REF(accept4);
BIND_REF(poll);
BIND_REF(getsockopt);
BIND_REF(dup2);
BIND_REF(dup);
BIND_REF(pipe);
BIND_REF(pipe2);
BIND_REF(pwrite);
BIND_REF(pread);
BIND_REF(readv);
BIND_REF(writev);

//XXX: This is ugly, for some reason things sometimes break strangely when this lives in
//its own object fil.

#include "events_gen.c"
