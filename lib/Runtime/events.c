
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
#include <pthread.h>

#include <sys/cdefs.h>
#include <sys/syscall.h>

// stat/fstat
#include <sys/stat.h>

// getrlimit, setrlimit
#include <sys/resource.h>

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

#include <libc_private.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "util.h"

extern void *__sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
extern interpos_func_t __libc_interposing[] __hidden;

extern void LogDone();

//XXX:convert
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

//XXX: convert
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
	    logData((uint8_t *)optval, *optlen);
	}
    }

    return result;
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
    int arg;

    va_start(ap, cmd);
    arg = va_arg(ap, int);
    va_end(ap);

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_fcntl, fd, cmd, arg);
	case RRMODE_RECORD:
	    ASSERT_IMPLEMENTED((cmd == F_GETFL) || (cmd == F_SETFL) || (cmd == F_SETFD));
	    result = syscall(SYS_fcntl, fd, cmd, arg);
	    RRRecordOI(RREVENT_FCNTL, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_FCNTL, &result);
	    break;
    }

    return result;
}

ssize_t
__rr_readlink(const char *restrict path, char *restrict buf, size_t bufsize)

{
    ssize_t result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_readlink, path, buf, bufsize);
	case RRMODE_RECORD:
	    result = syscall(SYS_readlink, path, buf, bufsize);
	    RRRecordOS(RREVENT_READLINK, 0, result);
	    if (result != -1) {
		logData((uint8_t*)buf, bufsize);
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayOS(RREVENT_READLINK, 0, &result);
	    if (result != -1) {
		logData((uint8_t*)buf, bufsize);
	    }
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

int
__rr_connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_connect, s, name, namelen);
	case RRMODE_RECORD:
	    result = syscall(SYS_connect, s, name, namelen);
	    RRRecordOI(RREVENT_CONNECT, s, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_CONNECT, &result);
	    break;
    }

    return result;
}

int
__rr_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_accept, s, addr, addrlen);
	case RRMODE_RECORD:
	    result = syscall(SYS_accept, s, addr, addrlen);
	    RRRecordOI(RREVENT_ACCEPT, s, result);
	    if (result != -1) {
		//XXX: slow, store in value[] directly
		logData((uint8_t*)addrlen, sizeof(*addrlen));
		logData((uint8_t*)addr, *addrlen);
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_ACCEPT, &result);
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
__rr_setsockopt(int s, int level, int optname, const void *optval, socklen_t 
	optlen)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_setsockopt, s, level, optname, optval, optlen);
	case RRMODE_RECORD:
	    result = syscall(SYS_setsockopt, s, level, optname, optval, optlen);
	    RRRecordOI(RREVENT_SETSOCKOPT, s, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SETSOCKOPT, &result);
	    break;
    }

    return result;
}

int
__rr_link(const char *name1, const char *name2)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_link, name1, name2);
	case RRMODE_RECORD:
	    result = syscall(SYS_link, name1, name2);
	    RRRecordI(RREVENT_LINK, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_LINK, &result);
	    break;
    }

    return result;
}

ssize_t
pread(int fd, void *buf, size_t nbytes, off_t offset)
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
__rr_symlink(const char *name1, const char *name2)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_symlink, name1, name2);
	case RRMODE_RECORD:
	    result = syscall(SYS_symlink, name1, name2);
	    RRRecordI(RREVENT_SYMLINK, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SYMLINK, &result);
	    break;
    }

    return result;
}

int
__rr_unlink(const char *path)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_unlink, path);
	case RRMODE_RECORD:
	    result = syscall(SYS_unlink, path);
	    RRRecordI(RREVENT_UNLINK, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_UNLINK, &result);
	    break;
    }

    return result;
}

int
__rr_rename(const char *name1, const char *name2)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_rename, name1, name2);
	case RRMODE_RECORD:
	    result = syscall(SYS_rename, name1, name2);
	    RRRecordI(RREVENT_RENAME, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_RENAME, &result);
	    break;
    }

    return result;
}

int
__rr_mkdir(const char *path, mode_t mode)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_mkdir, path, mode);
	case RRMODE_RECORD:
	    result = syscall(SYS_mkdir, path, mode);
	    RRRecordI(RREVENT_MKDIR, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_MKDIR, &result);
	    break;
    }

    return result;
}

int
__rr_rmdir(const char *path)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_rmdir, path);
	case RRMODE_RECORD:
	    result = syscall(SYS_rmdir, path);
	    RRRecordI(RREVENT_RMDIR, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_RMDIR, &result);
	    break;
    }

    return result;
}

int
__rr_chmod(const char *path, mode_t mode)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_chmod, path, mode);
	case RRMODE_RECORD:
	    result = syscall(SYS_chmod, path, mode);
	    RRRecordI(RREVENT_CHMOD, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_CHMOD, &result);
	    break;
    }

    return result;
}

int
__rr_access(const char *path, int mode)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_access, path, mode);
	case RRMODE_RECORD:
	    result = syscall(SYS_access, path, mode);
	    RRRecordI(RREVENT_ACCESS, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_ACCESS, &result);
	    break;
    }

    return result;
}

int
__rr_eaccess(const char *path, int mode)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_eaccess, path, mode);
	case RRMODE_RECORD:
	    result = syscall(SYS_eaccess, path, mode);
	    RRRecordI(RREVENT_EACCESS, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_EACCESS, &result);
	    break;
    }

    return result;
}

int
__rr_truncate(const char *path, off_t length)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_truncate, path, length);
	case RRMODE_RECORD:
	    result = syscall(SYS_truncate, path, length);
	    RRRecordI(RREVENT_TRUNCATE, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_TRUNCATE, &result);
	    break;
    }

    return result;
}

int
__rr_ftruncate(int fd, off_t length)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_ftruncate, fd, length);
	case RRMODE_RECORD:
	    result = syscall(SYS_ftruncate, fd, length);
	    RRRecordOI(RREVENT_FTRUNCATE, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_FTRUNCATE, &result);
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

int
__rr_fsync(int fd)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_fsync, fd);
	case RRMODE_RECORD:
	    result = syscall(SYS_fsync, fd);
	    RRRecordOI(RREVENT_FSYNC, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_FSYNC, &result);
	    break;
    }

    return result;
}

off_t
__rr_lseek(int fildes, off_t offset, int whence)
{
    off_t result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_lseek, fildes, offset, whence);
	case RRMODE_RECORD:
	    result = syscall(SYS_lseek, fildes, offset, whence);
	    RRRecordOS(RREVENT_LSEEK, fildes, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOS(RREVENT_LSEEK, fildes, &result);
	    break;
    }

    return result;
}

int
__rr_chdir(const char *path)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_chdir, path);
	case RRMODE_RECORD:
	    result = syscall(SYS_chdir, path);
	    RRRecordI(RREVENT_CHDIR, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_CHDIR, &result);
	    break;
    }

    return result;
}

int
__rr_fchdir(int fd)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_fchdir, fd);
	case RRMODE_RECORD:
	    result = syscall(SYS_fchdir, fd);
	    RRRecordOI(RREVENT_FCHDIR, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_FCHDIR, &result);
	    break;
    }

    return result;
}

int
__rr_lstat(const char *path, struct stat *sb)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_lstat, path, sb);
	case RRMODE_RECORD:
	    result = syscall(SYS_lstat, path, sb);
	    RRRecordI(RREVENT_LSTAT, result);
	    if (result == 0) {
		logData((uint8_t*)sb, sizeof(*sb));
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_LSTAT, &result);
	    if (result == 0) {
		logData((uint8_t*)sb, sizeof(*sb));
	    }
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

int
__rr_fstatat(int fd, const char *path, struct stat *buf, int flag)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_fstatat, fd, path, buf, flag);
	case RRMODE_RECORD:
	    result = syscall(SYS_fstatat, fd, path, buf, flag);
	    RRRecordOI(RREVENT_FSTATAT, fd, result);
	    if (result == 0) {
		logData((uint8_t*)buf, sizeof(*buf));
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_FSTATAT, &result);
	    if (result == 0) {
		logData((uint8_t*)buf, sizeof(*buf));
	    }
	    break;
    }

    return result;
}


int
__rr_fstat(int fd, struct stat *sb)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_fstat, fd, sb);
	case RRMODE_RECORD:
	    result = syscall(SYS_fstat, fd, sb);
	    RRRecordOI(RREVENT_FSTAT, fd, result);
	    if (result == 0) {
		logData((uint8_t*)sb, sizeof(*sb));
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_FSTAT, &result);
	    if (result == 0) {
		logData((uint8_t*)sb, sizeof(*sb));
	    }
	    break;
    }

    return result;
}

int
__rr_statfs(const char * path, struct statfs *buf)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_statfs, path, buf);
	case RRMODE_RECORD:
	    result = syscall(SYS_statfs, path, buf);
	    RRRecordI(RREVENT_STATFS, result);
	    if (result == 0) {
		logData((uint8_t*)buf, sizeof(*buf));
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_STATFS, &result);
	    if (result == 0) {
		logData((uint8_t*)buf, sizeof(*buf));
	    }
	    break;
    }

    return result;
}


int
__rr_fstatfs(int fd, struct statfs *buf)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_fstatfs, fd, buf);
	case RRMODE_RECORD:
	    result = syscall(SYS_fstatfs, fd, buf);
	    RRRecordOI(RREVENT_FSTATFS, fd, result);
	    if (result == 0) {
		logData((uint8_t*)buf, sizeof(*buf));
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_FSTATFS, &result);
	    if (result == 0) {
		logData((uint8_t*)buf, sizeof(*buf));
	    }
	    break;
    }

    return result;
}

int
__rr_stat(const char * restrict path, struct stat * restrict sb)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_stat, path, sb);
	case RRMODE_RECORD:
	    result = syscall(SYS_stat, path, sb);
	    RRRecordI(RREVENT_STAT, result);
	    if (result == 0) {
		logData((uint8_t*)sb, sizeof(*sb));
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_STAT, &result);
	    if (result == 0) {
		logData((uint8_t*)sb, sizeof(*sb));
	    }
	    break;
    }

    return result;
}

int
__rr_getrlimit(int resource, struct rlimit *rlp)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_getrlimit, resource, rlp);
	case RRMODE_RECORD:
	    result = syscall(SYS_getrlimit, resource, rlp);
	    RRRecordI(RREVENT_GETRLIMIT, result);
	    if (result == 0) {
		logData((uint8_t*)rlp, sizeof(*rlp));
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_GETRLIMIT, &result);
	    if (result == 0) {
		logData((uint8_t*)rlp, sizeof(*rlp));
	    }
	    break;
    }

    return result;
}

int
__rr_setrlimit(int resource, const struct rlimit *rlp)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_setrlimit, resource, rlp);
	case RRMODE_RECORD:
	    result = syscall(SYS_setrlimit, resource, rlp);
	    RRRecordI(RREVENT_SETRLIMIT, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SETRLIMIT, &result);
	    break;
    }

    return result;
}

int
__rr_getrusage(int who, struct rusage *rusage)
{
    int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_getrusage, who, rusage);
	case RRMODE_RECORD:
	    result = syscall(SYS_getrusage, who, rusage);
	    RRRecordI(RREVENT_GETRUSAGE, result);
	    if (result == 0) {
		logData((uint8_t*)rusage, sizeof(*rusage));
	    }
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_GETRUSAGE, &result);
	    if (result == 0) {
		logData((uint8_t*)rusage, sizeof(*rusage));
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

ssize_t
__rr_sendmsg(int s, const struct msghdr *msg, int flags)
{
    ssize_t result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_sendmsg, s, msg, flags);
	case RRMODE_RECORD:
	    result = syscall(SYS_sendmsg, s, msg, flags);
	    RRRecordOS(RREVENT_SENDMSG, s, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOS(RREVENT_SENDMSG, s, &result);
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
__rr_sendto(int s, const void *msg, size_t len, int flags,
		 const struct sockaddr *to, socklen_t tolen)
{
    ssize_t result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_sendto, s, msg, len, flags, to, tolen);
	case RRMODE_RECORD:
	    result = syscall(SYS_sendto, s, msg, len, flags, to, tolen);
	    RRRecordOS(RREVENT_SENDTO, s, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOS(RREVENT_SENDTO, s, &result);
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

void
Events_Init()
{
    __libc_interposing[INTERPOS_read] = (interpos_func_t)&__rr_read;
    __libc_interposing[INTERPOS_write] = (interpos_func_t)&__rr_write;
    __libc_interposing[INTERPOS_openat] = (interpos_func_t)&__rr_openat;
    __libc_interposing[INTERPOS_close] = (interpos_func_t)&__rr_close;
    __libc_interposing[INTERPOS_fcntl] = (interpos_func_t)&__rr_fcntl;
}

__strong_reference(__rr_read, _read);
__strong_reference(__rr_write, _write);
__strong_reference(__rr_close, _close);
__strong_reference(__rr_fcntl, _fcntl);

BIND_REF(stat);
BIND_REF(openat);
BIND_REF(fstat);
BIND_REF(statfs);
BIND_REF(fstatfs);
BIND_REF(fstatat);
BIND_REF(readlink);
BIND_REF(truncate);
BIND_REF(ftruncate);
BIND_REF(flock);
BIND_REF(fsync);
BIND_REF(lseek);
BIND_REF(lstat);
BIND_REF(umask);
BIND_REF(getrlimit);
BIND_REF(setrlimit);
BIND_REF(getrusage);
BIND_REF(getpeername);
BIND_REF(getsockname);
BIND_REF(sendto);
BIND_REF(sendmsg);
BIND_REF(select);
BIND_REF(recvmsg);
BIND_REF(recvfrom);
BIND_REF(mkdir);
BIND_REF(access);
BIND_REF(eaccess);
BIND_REF(chmod);
BIND_REF(fchdir);
BIND_REF(chdir);
BIND_REF(rmdir);
BIND_REF(link);
BIND_REF(symlink);
BIND_REF(unlink);
BIND_REF(rename);
BIND_REF(open);
BIND_REF(ioctl);
BIND_REF(setsockopt);
BIND_REF(socket);
BIND_REF(bind);
BIND_REF(listen);
BIND_REF(connect);
BIND_REF(accept);
BIND_REF(poll);
BIND_REF(getsockopt);

