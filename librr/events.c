
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>

#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>

#include <sys/cdefs.h>
#include <sys/syscall.h>

// sysctl
#include <sys/sysctl.h>

// socket
#include <sys/socket.h>

// poll/kqueue
#include <poll.h>
#include <sys/event.h>
#include <sys/time.h>

#include <libc_private.h>

#include "rrevent.h"
#include "rrlog.h"
#include "rrplay.h"
#include "rrgq.h"
#include "runtime.h"
#include "ft.h"

extern int
_pthread_create(pthread_t * thread, const pthread_attr_t * attr,
	       void *(*start_routine) (void *), void *arg);
extern int
_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
extern int _pthread_mutex_trylock(pthread_mutex_t *mutex);
extern int _pthread_mutex_lock(pthread_mutex_t *mutex);
extern int _pthread_mutex_unlock(pthread_mutex_t *mutex);
extern int _pthread_mutex_destroy(pthread_mutex_t *mutex);
extern int __vdso_clock_gettime(clockid_t clock_id, struct timespec *tp);
extern interpos_func_t __libc_interposing[] __hidden;

ssize_t __sys_read(int fd, void *buf, size_t nbytes);
ssize_t __sys_write(int fd, const void *buf, size_t nbytes);
int __clock_gettime(clockid_t clock_id, struct timespec *tp);
int __rr_sysctl(const int *name, u_int namelen, void *oldp,
	     size_t *oldlenp, const void *newp, size_t newlen);
void _rr_exit(int status);
int __socket(int domain, int type, int protocol);
int __bind(int s, const struct sockaddr *addr, socklen_t addrlen);
int __listen(int s, int backlog);
int __accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int __connect(int s, const struct sockaddr *name, socklen_t namelen);
int __poll(struct pollfd fds[], nfds_t nfds, int timeout);
int __kqueue();
int __kevent(int kq, const struct kevent *changelist, int nchanges,
	struct kevent *eventlist, int nevents,
	const struct timespec *timeout);

__strong_reference(__clock_gettime, clock_gettime);
__strong_reference(__rr_sysctl, __sysctl);
__strong_reference(_rr_exit, _exit);
__strong_reference(__socket, socket);
__strong_reference(__bind, bind);
__strong_reference(__listen, listen);
__strong_reference(__accept, accept);
__strong_reference(__connect, connect);
__strong_reference(__poll, poll);
__strong_reference(__kqueue, kqueue);
__strong_reference(__kevent, kevent);

void
Events_Init()
{
    __libc_interposing[INTERPOS_read] = (interpos_func_t)&__sys_read;
    __libc_interposing[INTERPOS_write] = (interpos_func_t)&__sys_write;
}

#if 1
void
AssertEvent(RRLogEntry *e, int evt)
{
    if (e->event != evt) {
	rrMode = RRMODE_NORMAL;
	printf("Expected %08x, Encountered %08x\n", evt, e->event);
	printf("Event #%lu, Thread #%d\n", e->eventId, e->threadId);
	printf("NextEvent #%lu, LastEvent #%lu\n", rrlog.nextEvent, rrlog.lastEvent);
	abort();
    }
}

void
AssertReplay(RRLogEntry *e, bool test)
{
    if (!test) {
	rrMode = RRMODE_NORMAL;
	printf("Encountered %08x\n", e->event);
	printf("Event #%lu, Thread #%d\n", e->eventId, e->threadId);
	printf("NextEvent #%lu, LastEvent #%lu\n", rrlog.nextEvent, rrlog.lastEvent);
	abort();
    }
}
#else
#define AssertEvent(_e, _evt)
#define AssertReplay(_e, _tst)
#endif

#define RREVENT_DATA_LEN	32
#define RREVENT_DATA_OFFSET	32

void
logData(uint8_t *buf, size_t len)
{
    int32_t i;
    int32_t recs = len / RREVENT_DATA_LEN;
    int32_t rlen = len % RREVENT_DATA_LEN;
    RRLogEntry *e;

    if (rrMode == RRMODE_RECORD) {
	for (i = 0; i < recs; i++) {
	    e = RRLog_Alloc(&rrlog, threadId);
	    e->event = RREVENT_DATA;
	    uint8_t *dst = ((uint8_t *)e) + RREVENT_DATA_OFFSET;
	    memcpy(dst, buf, RREVENT_DATA_LEN);
	    RRLog_Append(&rrlog, e);
	    buf += RREVENT_DATA_LEN;
	}
	if (rlen) {
	    e = RRLog_Alloc(&rrlog, threadId);
	    e->event = RREVENT_DATA;
	    uint8_t *dst = ((uint8_t *)e) + RREVENT_DATA_OFFSET;
	    memcpy(dst, buf, rlen);
	    RRLog_Append(&rrlog, e);
	    buf += rlen;
	}
    } else {
	for (i = 0; i < recs; i++) {
	    e = RRPlay_Dequeue(&rrlog, threadId);
	    AssertEvent(e, RREVENT_DATA);
	    uint8_t *src = ((uint8_t *)e) + RREVENT_DATA_OFFSET;
	    memcpy(buf, src, RREVENT_DATA_LEN);
	    RRPlay_Free(&rrlog, e);
	    buf += RREVENT_DATA_LEN;
	}
	if (rlen) {
	    e = RRPlay_Dequeue(&rrlog, threadId);
	    AssertEvent(e, RREVENT_DATA);
	    uint8_t *src = ((uint8_t *)e) + RREVENT_DATA_OFFSET;
	    memcpy(buf, src, rlen);
	    RRPlay_Free(&rrlog, e);
	    buf += rlen;
	}
    }
}

int
pthread_create(pthread_t * thread, const pthread_attr_t * attr,
	       void *(*start_routine) (void *), void *arg)
{
    int result;
    RRLogEntry *e;

    result = _pthread_create(thread, attr, start_routine, arg);

    if (rrMode == RRMODE_NORMAL) {
	return result;
    }

    if (rrMode == RRMODE_RECORD) {
	e = RRLog_Alloc(&rrlog, threadId);
	e->event = RREVENT_THREAD_CREATE;
	e->threadId = threadId;
	e->value[0] = result;
	RRLog_Append(&rrlog, e);
    } else {
	e = RRPlay_Dequeue(&rrlog, threadId);
	AssertReplay(e, result == e->value[0]);
	AssertEvent(e, RREVENT_THREAD_CREATE);
	RRPlay_Free(&rrlog, e);
    }

    return result;
}

int
pthread_mutex_init(pthread_mutex_t *mtx, const pthread_mutexattr_t *attr)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_Alloc(&rrlog, threadId);
	    e->event = RREVENT_MUTEX_INIT;
	    e->objectId = (uint64_t)mtx;
	    e->threadId = threadId;
	    RRLog_Append(&rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    break;
	case RRMODE_FASTRECORD:
	case RRMODE_FASTREPLAY:
	    break;
    }
    return _pthread_mutex_init(mtx, attr);
}

int
pthread_mutex_destroy(pthread_mutex_t *mtx)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_Alloc(&rrlog, threadId);
	    e->event = RREVENT_MUTEX_DESTROY;
	    e->objectId = (uint64_t)mtx;
	    e->threadId = threadId;
	    RRLog_Append(&rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    break;
	case RRMODE_FASTRECORD:
	case RRMODE_FASTREPLAY:
	    break;
    }

    return _pthread_mutex_destroy(mtx);
}

int
pthread_mutex_lock(pthread_mutex_t *mtx)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_mutex_lock(mtx);
	    break;
	}
	case RRMODE_RECORD: {
	    e = RRLog_Alloc(&rrlog, threadId);
	    e->event = RREVENT_MUTEX_LOCK;
	    e->objectId = (uint64_t)mtx;
	    e->threadId = threadId;
	    result = _pthread_mutex_lock(mtx);
	    RRLog_Append(&rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    e = RRPlay_Dequeue(&rrlog, threadId);
	    RRPlay_Free(&rrlog, e);
	    break;
	}
	case RRMODE_FASTRECORD: {
	    e = RRLog_Alloc(&rrlog, threadId);
	    e->objectId = 1;
	    e->threadId = threadId;
	    result = _pthread_mutex_lock(mtx);
	    RRLog_Append(&rrlog, e);
	    break;
	}
	case RRMODE_FASTREPLAY: {
	    e = RRPlay_Dequeue(&rrlog, threadId);
	    result = _pthread_mutex_lock(mtx);
	    RRPlay_Free(&rrlog, e);
	    break;
	}
    }

    return result;
}

int
pthread_mutex_trylock(pthread_mutex_t *mtx)
{
    int result = 0;
    RRLogEntry *e;

    abort();

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_mutex_trylock(mtx);
	    break;
	}
	case RRMODE_RECORD: {
	    e = RRLog_Alloc(&rrlog, threadId);
	    e->event = RREVENT_MUTEX_LOCK;
	    e->objectId = (uint64_t)mtx;
	    e->threadId = threadId;
	    result = _pthread_mutex_trylock(mtx);
	    e->value[0] = result;
	    RRLog_Append(&rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    e = RRPlay_Dequeue(&rrlog, threadId);
	    result = e->value[0];
	    RRPlay_Free(&rrlog, e);
	    break;
	}
	case RRMODE_FASTRECORD: {
	    e = RRLog_Alloc(&rrlog, threadId);
	    e->objectId = 1;
	    e->threadId = threadId;
	    result = _pthread_mutex_trylock(mtx);
	    RRLog_Append(&rrlog, e);
	    break;
	}
	case RRMODE_FASTREPLAY: {
	    e = RRPlay_Dequeue(&rrlog, threadId);
	    result = _pthread_mutex_trylock(mtx);
	    RRPlay_Free(&rrlog, e);
	    break;
	}
    }

    return result;
}

int
pthread_mutex_unlock(pthread_mutex_t *mtx)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_mutex_unlock(mtx);
	    break;
	}
	case RRMODE_RECORD: {
	    e = RRLog_Alloc(&rrlog, threadId);
	    e->event = RREVENT_MUTEX_UNLOCK;
	    e->objectId = (uint64_t)mtx;
	    e->threadId = threadId;
	    result = _pthread_mutex_unlock(mtx);
	    RRLog_Append(&rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    e = RRPlay_Dequeue(&rrlog, threadId);
	    RRPlay_Free(&rrlog, e);
	    break;
	}
	case RRMODE_FASTRECORD: {
	    result = _pthread_mutex_unlock(mtx);
	    break;
	}
	case RRMODE_FASTREPLAY: {
	    result = _pthread_mutex_unlock(mtx);
	    break;
	}
    }

    return result;
}

ssize_t
__sys_read(int fd, void *buf, size_t nbytes)
{
    ssize_t result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS_read, fd, buf, nbytes);
    }

    if (rrMode == RRMODE_RECORD) {
	int left;
	result = syscall(SYS_read, fd, buf, nbytes);

	e = RRLog_Alloc(&rrlog, threadId);
	e->event = RREVENT_READ;
	e->threadId = threadId;
	e->value[0] = result;
	RRLog_Append(&rrlog, e);

	if (result > 0) {
	    logData(buf, result);
	}
    } else {
	e = RRPlay_Dequeue(&rrlog, threadId);
	result = e->value[0];
	AssertEvent(e, RREVENT_READ);
	RRPlay_Free(&rrlog, e);

	if (result > 0) {
	    logData(buf, result);
	}
    }

    return result;
}

ssize_t
_read(int fd, void *buf, size_t nbytes)
{
    return read(fd, buf, nbytes);
}

ssize_t
__sys_write(int fd, const void *buf, size_t nbytes)
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS_write, fd, buf, nbytes);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS_write, fd, buf, nbytes);

	e = RRLog_Alloc(&rrlog, threadId);
	e->event = RREVENT_WRITE;
	e->threadId = threadId;
	e->objectId = fd;
	e->value[0] = result;
	RRLog_Append(&rrlog, e);
    } else {
	if (fd == 1) {
	    // Print console output
	    syscall(SYS_write, fd, buf, nbytes);
	}

	e = RRPlay_Dequeue(&rrlog, threadId);
	AssertEvent(e, RREVENT_WRITE);
	result = e->value[0];
	RRPlay_Free(&rrlog, e);
    }

    return result;
}

ssize_t
_write(int fd, const void *buf, size_t nbytes)
{
    return write(fd, buf, nbytes);
}

int
__clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return __vdso_clock_gettime(clock_id, tp);
    }

    if (rrMode == RRMODE_RECORD) {
	result = __vdso_clock_gettime(clock_id, tp);

	e = RRLog_Alloc(&rrlog, threadId);
	e->event = RREVENT_GETTIME;
	e->threadId = threadId;
	e->objectId = clock_id;
	e->value[0] = tp->tv_sec;
	e->value[1] = tp->tv_nsec;
	e->value[2] = result;
	RRLog_Append(&rrlog, e);
    } else {
	e = RRPlay_Dequeue(&rrlog, threadId);
	AssertEvent(e, RREVENT_GETTIME);
	tp->tv_sec = e->value[0];
	tp->tv_nsec = e->value[1];
	result = e->value[2];
	RRPlay_Free(&rrlog, e);
    }

    return result;
}

int __rr_sysctl(const int *name, u_int namelen, void *oldp,
	     size_t *oldlenp, const void *newp, size_t newlen)
{
    ssize_t result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS___sysctl, name, namelen, oldp, oldlenp, newp, newlen);
    }

    if (rrMode == RRMODE_RECORD) {
	int left;
	result = syscall(SYS___sysctl, name, namelen, oldp, oldlenp, newp, newlen);

	e = RRLog_Alloc(&rrlog, threadId);
	e->event = RREVENT_SYSCTL;
	e->threadId = threadId;
	e->value[0] = result;
	e->value[1] = *oldlenp;
	RRLog_Append(&rrlog, e);

	if (oldp) {
	    logData(oldp, *oldlenp);
	}
    } else {
	e = RRPlay_Dequeue(&rrlog, threadId);
	result = e->value[0];
	if (oldp) {
	    *oldlenp = e->value[1];
	}
	AssertEvent(e, RREVENT_SYSCTL);
	RRPlay_Free(&rrlog, e);

	if (oldp) {
	    logData(oldp, *oldlenp);
	}
    }

    return result;
}

void
_rr_exit(int status)
{
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	syscall(SYS_exit, status);
	__builtin_unreachable();
    }

    if (rrMode == RRMODE_RECORD) {
	e = RRLog_Alloc(&rrlog, threadId);
	e->event = RREVENT_EXIT;
	e->threadId = threadId;
	RRLog_Append(&rrlog, e);
    } else {
	e = RRPlay_Dequeue(&rrlog, threadId);
	AssertEvent(e, RREVENT_EXIT);
	RRPlay_Free(&rrlog, e);
    }

    // Drain log
    LogDone();

    syscall(SYS_exit, status);
}

int
__socket(int domain, int type, int protocol)
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS_socket, domain, type, protocol);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS_socket, domain, type, protocol);
	e = RRLog_Alloc(&rrlog, threadId);
	e->event = RREVENT_SOCKET;
	e->threadId = threadId;
	e->value[0] = result;
	RRLog_Append(&rrlog, e);
    } else {
	e = RRPlay_Dequeue(&rrlog, threadId);
	AssertEvent(e, RREVENT_SOCKET);
	result = e->value[0];
	RRPlay_Free(&rrlog, e);
    }

    return result;
}

int
__bind(int s, const struct sockaddr *addr, socklen_t addrlen)
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS_bind, s, addr, addrlen);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS_bind, s, addr, addrlen);
	e = RRLog_Alloc(&rrlog, threadId);
	e->event = RREVENT_BIND;
	e->threadId = threadId;
	e->value[0] = result;
	RRLog_Append(&rrlog, e);
    } else {
	e = RRPlay_Dequeue(&rrlog, threadId);
	AssertEvent(e, RREVENT_BIND);
	result = e->value[0];
	RRPlay_Free(&rrlog, e);
    }

    return result;
}

int
__listen(int s, int backlog)
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS_listen, s, backlog);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS_listen, s, backlog);
	e = RRLog_Alloc(&rrlog, threadId);
	e->event = RREVENT_LISTEN;
	e->threadId = threadId;
	e->value[0] = result;
	RRLog_Append(&rrlog, e);
    } else {
	e = RRPlay_Dequeue(&rrlog, threadId);
	AssertEvent(e, RREVENT_LISTEN);
	result = e->value[0];
	RRPlay_Free(&rrlog, e);
    }

    return result;
}

int
__connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    // TODO
    return -1;
}

int
__accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS_accept, s, addr, addrlen);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS_accept, s, addr, addrlen);
	e = RRLog_Alloc(&rrlog, threadId);
	e->event = RREVENT_ACCEPT;
	e->threadId = threadId;
	e->value[0] = result;
	e->value[1] = *addrlen;
	RRLog_Append(&rrlog, e);

	if (result >= 0) {
	    logData((uint8_t *)addr, *addrlen);
	}
    } else {
	e = RRPlay_Dequeue(&rrlog, threadId);
	AssertEvent(e, RREVENT_ACCEPT);
	result = e->value[0];
	if (result >= 0) {
	    *addrlen = e->value[1];
	}
	RRPlay_Free(&rrlog, e);

	if (result >= 0) {
	    logData((uint8_t *)addr, *addrlen);
	}
    }

    return result;
}

int
__poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS_poll, fds, nfds, timeout);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS_poll, fds, nfds, timeout);
	e = RRLog_Alloc(&rrlog, threadId);
	e->event = RREVENT_POLL;
	e->threadId = threadId;
	e->value[0] = result;
	e->value[1] = nfds;
	RRLog_Append(&rrlog, e);
	if (result > 0) {
	    logData((uint8_t *)fds, nfds * sizeof(struct pollfd));
	}
    } else {
	e = RRPlay_Dequeue(&rrlog, threadId);
	AssertEvent(e, RREVENT_POLL);
	result = e->value[0];
	assert(e->value[1] == nfds);
	RRPlay_Free(&rrlog, e);
	if (result > 0) {
	    logData((uint8_t *)fds, nfds * sizeof(struct pollfd));
	}
    }

    return result;
}

int
__kqueue()
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS_kqueue);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS_kqueue);
	e = RRLog_Alloc(&rrlog, threadId);
	e->event = RREVENT_KQUEUE;
	e->threadId = threadId;
	e->value[0] = result;
	RRLog_Append(&rrlog, e);
    } else {
	e = RRPlay_Dequeue(&rrlog, threadId);
	AssertEvent(e, RREVENT_KQUEUE);
	result = e->value[0];
	RRPlay_Free(&rrlog, e);
    }

    return result;
}

int
__kevent(int kq, const struct kevent *changelist, int nchanges,
	struct kevent *eventlist, int nevents,
	const struct timespec *timeout)
{
    int result;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS_kevent, changelist, nchanges,
		       eventlist, nevents, timeout);
    }

    if (rrMode == RRMODE_RECORD) {
	result = syscall(SYS_kevent, changelist, nchanges,
			 eventlist, nevents, timeout);
	e = RRLog_Alloc(&rrlog, threadId);
	e->event = RREVENT_KEVENT;
	e->threadId = threadId;
	e->value[0] = result;
	e->value[1] = nchanges;
	e->value[2] = nevents;
	RRLog_Append(&rrlog, e);
	if (result > 0) {
	    logData((uint8_t *)eventlist, sizeof(struct kevent) * result);
	}
    } else {
	e = RRPlay_Dequeue(&rrlog, threadId);
	AssertEvent(e, RREVENT_KEVENT);
	result = e->value[0];
	RRPlay_Free(&rrlog, e);
	if (result > 0) {
	    logData((uint8_t *)eventlist, sizeof(struct kevent) * result);
	}
    }

    return result;
}

