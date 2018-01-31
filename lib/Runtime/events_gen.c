
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















ssize_t
__rr_read(int fd, void * buf, size_t nbyte)
{
	ssize_t result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_read, fd, buf, nbyte);
	case RRMODE_RECORD:
	    result = syscall(SYS_read, fd, buf, nbyte);
	    RRRecordOS(RREVENT_READ, fd, result);
		if (result != -1) {
			logData((uint8_t *)buf, nbyte);
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayOS(RREVENT_READ, fd, &result);
		if (result != -1) {
			logData((uint8_t *)buf, nbyte);
		}
	    break;
    }
    return result;
}

int
__rr_link(const char * path, const char * to)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_link, path, to);
	case RRMODE_RECORD:
	    result = syscall(SYS_link, path, to);
	    RRRecordI(RREVENT_LINK, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_LINK, &result);
	    break;
    }
    return result;
}

int
__rr_unlink(const char * path)
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
__rr_chdir(const char * path)
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
	    RRReplayOI(RREVENT_FCHDIR, fd, &result);
	    break;
    }
    return result;
}

int
__rr_chmod(const char * path, mode_t mode)
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
__rr_setuid(uid_t uid)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_setuid, uid);
	case RRMODE_RECORD:
	    result = syscall(SYS_setuid, uid);
	    RRRecordI(RREVENT_SETUID, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SETUID, &result);
	    break;
    }
    return result;
}

ssize_t
__rr_sendmsg(int s, const struct msghdr * msg, int flags)
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

int
__rr_accept(int s, struct sockaddr * __restrict name, __socklen_t * anamelen)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_accept, s, name, anamelen);
	case RRMODE_RECORD:
	    result = syscall(SYS_accept, s, name, anamelen);
	    RRRecordOI(RREVENT_ACCEPT, s, result);
		if (result != -1) {
			 if (name != NULL) {
			logData((uint8_t *)name, *anamelen);
			 }
			 if (anamelen != NULL) {
			logData((uint8_t *)anamelen, sizeof(__socklen_t));
			 }
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_ACCEPT, s, &result);
		if (result != -1) {
			 if (name != NULL) {
			logData((uint8_t *)name, *anamelen);
			 }
			 if (anamelen != NULL) {
			logData((uint8_t *)anamelen, sizeof(__socklen_t));
			 }
		}
	    break;
    }
    return result;
}

int
__rr_getpeername(int fdes, struct sockaddr * __restrict asa, __socklen_t * alen)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_getpeername, fdes, asa, alen);
	case RRMODE_RECORD:
	    result = syscall(SYS_getpeername, fdes, asa, alen);
	    RRRecordOI(RREVENT_GETPEERNAME, fdes, result);
		if (result == 0) {
			logData((uint8_t *)asa, *alen);
			 if (alen != NULL) {
			logData((uint8_t *)alen, sizeof(__socklen_t));
			 }
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_GETPEERNAME, fdes, &result);
		if (result == 0) {
			logData((uint8_t *)asa, *alen);
			 if (alen != NULL) {
			logData((uint8_t *)alen, sizeof(__socklen_t));
			 }
		}
	    break;
    }
    return result;
}

int
__rr_getsockname(int fdes, struct sockaddr * __restrict asa, __socklen_t * alen)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_getsockname, fdes, asa, alen);
	case RRMODE_RECORD:
	    result = syscall(SYS_getsockname, fdes, asa, alen);
	    RRRecordOI(RREVENT_GETSOCKNAME, fdes, result);
		if (result == 0) {
			logData((uint8_t *)asa, *alen);
			logData((uint8_t *)alen, sizeof(__socklen_t));
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_GETSOCKNAME, fdes, &result);
		if (result == 0) {
			logData((uint8_t *)asa, *alen);
			logData((uint8_t *)alen, sizeof(__socklen_t));
		}
	    break;
    }
    return result;
}

int
__rr_access(const char * path, int amode)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_access, path, amode);
	case RRMODE_RECORD:
	    result = syscall(SYS_access, path, amode);
	    RRRecordI(RREVENT_ACCESS, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_ACCESS, &result);
	    break;
    }
    return result;
}

int
__rr_stat(const char * path, struct stat * ub)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_stat, path, ub);
	case RRMODE_RECORD:
	    result = syscall(SYS_stat, path, ub);
	    RRRecordI(RREVENT_STAT, result);
		if (result == 0) {
			logData((uint8_t *)ub, sizeof(struct stat));
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_STAT, &result);
		if (result == 0) {
			logData((uint8_t *)ub, sizeof(struct stat));
		}
	    break;
    }
    return result;
}

int
__rr_lstat(const char * path, struct stat * ub)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_lstat, path, ub);
	case RRMODE_RECORD:
	    result = syscall(SYS_lstat, path, ub);
	    RRRecordI(RREVENT_LSTAT, result);
		if (result == 0) {
			logData((uint8_t *)ub, sizeof(struct stat));
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_LSTAT, &result);
		if (result == 0) {
			logData((uint8_t *)ub, sizeof(struct stat));
		}
	    break;
    }
    return result;
}

int
__rr_symlink(const char * path, const char * link)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_symlink, path, link);
	case RRMODE_RECORD:
	    result = syscall(SYS_symlink, path, link);
	    RRRecordI(RREVENT_SYMLINK, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SYMLINK, &result);
	    break;
    }
    return result;
}

ssize_t
__rr_readlink(const char * path, char * buf, size_t count)
{
	ssize_t result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_readlink, path, buf, count);
	case RRMODE_RECORD:
	    result = syscall(SYS_readlink, path, buf, count);
	    RRRecordS(RREVENT_READLINK, result);
		if (result != -1) {
			logData((uint8_t *)buf, count * sizeof(char));
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayS(RREVENT_READLINK, &result);
		if (result != -1) {
			logData((uint8_t *)buf, count * sizeof(char));
		}
	    break;
    }
    return result;
}

int
__rr_setgroups(int gidsetsize, const gid_t * gidset)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_setgroups, gidsetsize, gidset);
	case RRMODE_RECORD:
	    result = syscall(SYS_setgroups, gidsetsize, gidset);
	    RRRecordOI(RREVENT_SETGROUPS, gidsetsize, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_SETGROUPS, gidsetsize, &result);
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
	    RRReplayOI(RREVENT_FSYNC, fd, &result);
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
	    RRRecordOI(RREVENT_SOCKET, domain, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_SOCKET, domain, &result);
	    break;
    }
    return result;
}

int
__rr_connect(int s, const struct sockaddr * name, __socklen_t namelen)
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
	    RRReplayOI(RREVENT_CONNECT, s, &result);
	    break;
    }
    return result;
}

int
__rr_bind(int s, const struct sockaddr * name, __socklen_t namelen)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_bind, s, name, namelen);
	case RRMODE_RECORD:
	    result = syscall(SYS_bind, s, name, namelen);
	    RRRecordOI(RREVENT_BIND, s, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_BIND, s, &result);
	    break;
    }
    return result;
}

int
__rr_setsockopt(int s, int level, int name, const void * val, __socklen_t valsize)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_setsockopt, s, level, name, val, valsize);
	case RRMODE_RECORD:
	    result = syscall(SYS_setsockopt, s, level, name, val, valsize);
	    RRRecordOI(RREVENT_SETSOCKOPT, s, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_SETSOCKOPT, s, &result);
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
	    RRReplayOI(RREVENT_LISTEN, s, &result);
	    break;
    }
    return result;
}

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
__rr_fchmod(int fd, mode_t mode)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_fchmod, fd, mode);
	case RRMODE_RECORD:
	    result = syscall(SYS_fchmod, fd, mode);
	    RRRecordOI(RREVENT_FCHMOD, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_FCHMOD, fd, &result);
	    break;
    }
    return result;
}

int
__rr_rename(const char * from, const char * to)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_rename, from, to);
	case RRMODE_RECORD:
	    result = syscall(SYS_rename, from, to);
	    RRRecordI(RREVENT_RENAME, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_RENAME, &result);
	    break;
    }
    return result;
}

int
__rr_flock(int fd, int how)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_flock, fd, how);
	case RRMODE_RECORD:
	    result = syscall(SYS_flock, fd, how);
	    RRRecordOI(RREVENT_FLOCK, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_FLOCK, fd, &result);
	    break;
    }
    return result;
}

ssize_t
__rr_sendto(int s, const void * buf, size_t len, int flags, const struct sockaddr * to, __socklen_t tolen)
{
	ssize_t result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_sendto, s, buf, len, flags, to, tolen);
	case RRMODE_RECORD:
	    result = syscall(SYS_sendto, s, buf, len, flags, to, tolen);
	    RRRecordOS(RREVENT_SENDTO, s, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOS(RREVENT_SENDTO, s, &result);
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
__rr_mkdir(const char * path, mode_t mode)
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
__rr_rmdir(const char * path)
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
__rr_setgid(gid_t gid)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_setgid, gid);
	case RRMODE_RECORD:
	    result = syscall(SYS_setgid, gid);
	    RRRecordI(RREVENT_SETGID, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SETGID, &result);
	    break;
    }
    return result;
}

int
__rr_setegid(gid_t egid)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_setegid, egid);
	case RRMODE_RECORD:
	    result = syscall(SYS_setegid, egid);
	    RRRecordI(RREVENT_SETEGID, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SETEGID, &result);
	    break;
    }
    return result;
}

int
__rr_seteuid(uid_t euid)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_seteuid, euid);
	case RRMODE_RECORD:
	    result = syscall(SYS_seteuid, euid);
	    RRRecordI(RREVENT_SETEUID, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SETEUID, &result);
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
__rr_lchmod(const char * path, mode_t mode)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_lchmod, path, mode);
	case RRMODE_RECORD:
	    result = syscall(SYS_lchmod, path, mode);
	    RRRecordI(RREVENT_LCHMOD, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_LCHMOD, &result);
	    break;
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
__rr_eaccess(const char * path, int amode)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_eaccess, path, amode);
	case RRMODE_RECORD:
	    result = syscall(SYS_eaccess, path, amode);
	    RRRecordI(RREVENT_EACCESS, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_EACCESS, &result);
	    break;
    }
    return result;
}

ssize_t
__rr_pread(int fd, void * buf, size_t nbyte, off_t offset)
{
	ssize_t result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_pread, fd, buf, nbyte, offset);
	case RRMODE_RECORD:
	    result = syscall(SYS_pread, fd, buf, nbyte, offset);
	    RRRecordOS(RREVENT_PREAD, fd, result);
		if (result != -1) {
			logData((uint8_t *)buf, nbyte);
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayOS(RREVENT_PREAD, fd, &result);
		if (result != -1) {
			logData((uint8_t *)buf, nbyte);
		}
	    break;
    }
    return result;
}

off_t
__rr_lseek(int fd, off_t offset, int whence)
{
	off_t result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_lseek, fd, offset, whence);
	case RRMODE_RECORD:
	    result = syscall(SYS_lseek, fd, offset, whence);
	    RRRecordOS(RREVENT_LSEEK, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOS(RREVENT_LSEEK, fd, &result);
	    break;
    }
    return result;
}

int
__rr_truncate(const char * path, off_t length)
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
	    RRReplayOI(RREVENT_FTRUNCATE, fd, &result);
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
__rr_faccessat(int fd, const char * path, int amode, int flag)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_faccessat, fd, path, amode, flag);
	case RRMODE_RECORD:
	    result = syscall(SYS_faccessat, fd, path, amode, flag);
	    RRRecordOI(RREVENT_FACCESSAT, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_FACCESSAT, fd, &result);
	    break;
    }
    return result;
}

int
__rr_fchmodat(int fd, const char * path, mode_t mode, int flag)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_fchmodat, fd, path, mode, flag);
	case RRMODE_RECORD:
	    result = syscall(SYS_fchmodat, fd, path, mode, flag);
	    RRRecordOI(RREVENT_FCHMODAT, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_FCHMODAT, fd, &result);
	    break;
    }
    return result;
}

int
__rr_linkat(int fd1, const char * path1, int fd2, const char * path2, int flag)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_linkat, fd1, path1, fd2, path2, flag);
	case RRMODE_RECORD:
	    result = syscall(SYS_linkat, fd1, path1, fd2, path2, flag);
	    RRRecordOI(RREVENT_LINKAT, fd1, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_LINKAT, fd1, &result);
	    break;
    }
    return result;
}

ssize_t
__rr_readlinkat(int fd, const char * path, char * buf, size_t bufsize)
{
	ssize_t result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_readlinkat, fd, path, buf, bufsize);
	case RRMODE_RECORD:
	    result = syscall(SYS_readlinkat, fd, path, buf, bufsize);
	    RRRecordOS(RREVENT_READLINKAT, fd, result);
		if (result != -1) {
			logData((uint8_t *)buf, bufsize);
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayOS(RREVENT_READLINKAT, fd, &result);
		if (result != -1) {
			logData((uint8_t *)buf, bufsize);
		}
	    break;
    }
    return result;
}

int
__rr_unlinkat(int fd, const char * path, int flag)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_unlinkat, fd, path, flag);
	case RRMODE_RECORD:
	    result = syscall(SYS_unlinkat, fd, path, flag);
	    RRRecordOI(RREVENT_UNLINKAT, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_UNLINKAT, fd, &result);
	    break;
    }
    return result;
}

int
__rr_cap_enter()
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_cap_enter);
	case RRMODE_RECORD:
	    result = syscall(SYS_cap_enter);
	    RRRecordI(RREVENT_CAP_ENTER, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_CAP_ENTER, &result);
	    break;
    }
    return result;
}

int
__rr_cap_rights_limit(int fd, cap_rights_t * rightsp)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_cap_rights_limit, fd, rightsp);
	case RRMODE_RECORD:
	    result = syscall(SYS_cap_rights_limit, fd, rightsp);
	    RRRecordOI(RREVENT_CAP_RIGHTS_LIMIT, fd, result);
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_CAP_RIGHTS_LIMIT, fd, &result);
	    break;
    }
    return result;
}

int
__rr_accept4(int s, struct sockaddr * __restrict name, __socklen_t * __restrict anamelen, int flags)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_accept4, s, name, anamelen, flags);
	case RRMODE_RECORD:
	    result = syscall(SYS_accept4, s, name, anamelen, flags);
	    RRRecordOI(RREVENT_ACCEPT4, s, result);
		if (result != -1) {
			 if (name != NULL) {
			logData((uint8_t *)name, *anamelen);
			 }
			 if (anamelen != NULL) {
			logData((uint8_t *)anamelen, sizeof(__socklen_t));
			 }
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_ACCEPT4, s, &result);
		if (result != -1) {
			 if (name != NULL) {
			logData((uint8_t *)name, *anamelen);
			 }
			 if (anamelen != NULL) {
			logData((uint8_t *)anamelen, sizeof(__socklen_t));
			 }
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

int
__rr_fstatat(int fd, const char * path, struct stat * buf, int flag)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_fstatat, fd, path, buf, flag);
	case RRMODE_RECORD:
	    result = syscall(SYS_fstatat, fd, path, buf, flag);
	    RRRecordOI(RREVENT_FSTATAT, fd, result);
		if (result == 0) {
			logData((uint8_t *)buf, sizeof(struct stat));
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_FSTATAT, fd, &result);
		if (result == 0) {
			logData((uint8_t *)buf, sizeof(struct stat));
		}
	    break;
    }
    return result;
}

int
__rr_statfs(const char * path, struct statfs * buf)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_statfs, path, buf);
	case RRMODE_RECORD:
	    result = syscall(SYS_statfs, path, buf);
	    RRRecordI(RREVENT_STATFS, result);
		if (result == 0) {
			logData((uint8_t *)buf, sizeof(struct statfs));
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_STATFS, &result);
		if (result == 0) {
			logData((uint8_t *)buf, sizeof(struct statfs));
		}
	    break;
    }
    return result;
}

int
__rr_fstatfs(int fd, struct statfs * buf)
{
	int result;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return syscall(SYS_fstatfs, fd, buf);
	case RRMODE_RECORD:
	    result = syscall(SYS_fstatfs, fd, buf);
	    RRRecordOI(RREVENT_FSTATFS, fd, result);
		if (result == 0) {
			logData((uint8_t *)buf, sizeof(struct statfs));
		}
	    break;
	case RRMODE_REPLAY:
	    RRReplayOI(RREVENT_FSTATFS, fd, &result);
		if (result == 0) {
			logData((uint8_t *)buf, sizeof(struct statfs));
		}
	    break;
    }
    return result;
}

BIND_REF(read);
BIND_REF(link);
BIND_REF(unlink);
BIND_REF(chdir);
BIND_REF(fchdir);
BIND_REF(chmod);
BIND_REF(setuid);
BIND_REF(sendmsg);
BIND_REF(accept);
BIND_REF(getpeername);
BIND_REF(getsockname);
BIND_REF(access);
BIND_REF(stat);
BIND_REF(lstat);
BIND_REF(symlink);
BIND_REF(readlink);
BIND_REF(setgroups);
BIND_REF(fsync);
BIND_REF(socket);
BIND_REF(connect);
BIND_REF(bind);
BIND_REF(setsockopt);
BIND_REF(listen);
BIND_REF(getrusage);
BIND_REF(fchmod);
BIND_REF(rename);
BIND_REF(flock);
BIND_REF(sendto);
BIND_REF(shutdown);
BIND_REF(mkdir);
BIND_REF(rmdir);
BIND_REF(setgid);
BIND_REF(setegid);
BIND_REF(seteuid);
BIND_REF(getrlimit);
BIND_REF(setrlimit);
BIND_REF(lchmod);
BIND_REF(kqueue);
BIND_REF(eaccess);
BIND_REF(pread);
BIND_REF(lseek);
BIND_REF(truncate);
BIND_REF(ftruncate);
BIND_REF(cpuset);
BIND_REF(cpuset_setid);
BIND_REF(cpuset_getid);
BIND_REF(cpuset_getaffinity);
BIND_REF(cpuset_setaffinity);
BIND_REF(faccessat);
BIND_REF(fchmodat);
BIND_REF(linkat);
BIND_REF(readlinkat);
BIND_REF(unlinkat);
BIND_REF(cap_enter);
BIND_REF(cap_rights_limit);
BIND_REF(accept4);
BIND_REF(fstat);
BIND_REF(fstatat);
BIND_REF(statfs);
BIND_REF(fstatfs);
