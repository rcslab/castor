
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

#include <sys/syscall.h>

#include <libc_private.h>

#include "rrevent.h"
#include "rrlog.h"
#include "rrplay.h"
#include "rrgq.h"
#include "runtime.h"
#include "ft.h"

int
RRMutex_Init(pthread_mutex_t *mtx, pthread_mutexattr_t *attr)
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
    return pthread_mutex_init(mtx, attr);
}

int
RRMutex_Destroy(pthread_mutex_t *mtx)
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
    return pthread_mutex_destroy(mtx);
}

int
RRMutex_Lock(pthread_mutex_t *mtx)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = pthread_mutex_lock(mtx);
	    break;
	}
	case RRMODE_RECORD: {
	    e = RRLog_Alloc(&rrlog, threadId);
	    e->event = RREVENT_MUTEX_LOCK;
	    e->objectId = (uint64_t)mtx;
	    e->threadId = threadId;
	    result = pthread_mutex_lock(mtx);
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
	    result = pthread_mutex_lock(mtx);
	    RRLog_Append(&rrlog, e);
	    break;
	}
	case RRMODE_FASTREPLAY: {
	    e = RRPlay_Dequeue(&rrlog, threadId);
	    result = pthread_mutex_lock(mtx);
	    RRPlay_Free(&rrlog, e);
	    break;
	}
    }

    return result;
}

int
RRMutex_TryLock(pthread_mutex_t *mtx)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = pthread_mutex_lock(mtx);
	    break;
	}
	case RRMODE_RECORD: {
	    e = RRLog_Alloc(&rrlog, threadId);
	    e->event = RREVENT_MUTEX_LOCK;
	    e->objectId = (uint64_t)mtx;
	    e->threadId = threadId;
	    result = pthread_mutex_trylock(mtx);
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
	    result = pthread_mutex_lock(mtx);
	    RRLog_Append(&rrlog, e);
	    break;
	}
	case RRMODE_FASTREPLAY: {
	    e = RRPlay_Dequeue(&rrlog, threadId);
	    result = pthread_mutex_trylock(mtx);
	    RRPlay_Free(&rrlog, e);
	    break;
	}
    }

    return result;
}

int
RRMutex_Unlock(pthread_mutex_t *mtx)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = pthread_mutex_unlock(mtx);
	    break;
	}
	case RRMODE_RECORD: {
	    e = RRLog_Alloc(&rrlog, threadId);
	    e->event = RREVENT_MUTEX_UNLOCK;
	    e->objectId = (uint64_t)mtx;
	    e->threadId = threadId;
	    result = pthread_mutex_unlock(mtx);
	    RRLog_Append(&rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    e = RRPlay_Dequeue(&rrlog, threadId);
	    RRPlay_Free(&rrlog, e);
	    break;
	}
	case RRMODE_FASTRECORD: {
	    result = pthread_mutex_unlock(mtx);
	    break;
	}
	case RRMODE_FASTREPLAY: {
	    result = pthread_mutex_unlock(mtx);
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

    //fprintf(stderr, "Hello\n");

    if (rrMode == RRMODE_NORMAL) {
	return syscall(SYS_read, fd, buf, nbytes);
    }

    if (fd != 0) {
	return syscall(SYS_read, fd, buf, nbytes);
    }

    if (rrMode == RRMODE_RECORD) {
	int left;
	result = syscall(SYS_read, fd, buf, nbytes);
	//fprintf(stderr, "RECORD\n");

	// write log entry
	left = result;
	// XXX: check result is positive
	while (left > 0) {
	    e = RRLog_Alloc(&rrlog, threadId);
	    e->event = left <= 32 ? RREVENT_READEND : RREVENT_READ;
	    e->threadId = threadId;
	    memcpy((void *)&e->value[1], buf, left < 32 ? left : 32);
	    e->value[0] = left < 32 ? left : 32;
	    RRLog_Append(&rrlog, e);
	    left -= 32;
	    buf += 32;
	}
    } else {
	int result = 0;
	//fprintf(stderr, "REPLAY\n");

	// read log entry
	while (1) {
	    e = RRPlay_Dequeue(&rrlog, threadId);
	    memcpy(buf, (void *)&e->value[1], e->value[0]);
	    result += e->value[0];
	    if (e->event == RREVENT_READEND) {
		RRPlay_Free(&rrlog, e);
		return result;
	    }
	    RRPlay_Free(&rrlog, e);
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
    ssize_t result;
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
	int result = 0;

	e = RRPlay_Dequeue(&rrlog, threadId);
	result = e->value[0];
	RRPlay_Free(&rrlog, e);
	return result;
    }

    return result;
}

ssize_t
_write(int fd, const void *buf, size_t nbytes)
{
    return write(fd, buf, nbytes);
}

extern interpos_func_t __libc_interposing[] __hidden;

void
Events_Init()
{
    __libc_interposing[INTERPOS_read] = (interpos_func_t)&__sys_read;
    __libc_interposing[INTERPOS_write] = (interpos_func_t)&__sys_write;
}

