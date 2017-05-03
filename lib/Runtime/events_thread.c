
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

// wait/waitpid
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <libc_private.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "util.h"

extern pid_t __sys_getpid();
extern pid_t __sys_getppid();

extern int
_pthread_create(pthread_t * thread, const pthread_attr_t * attr,
	       void *(*start_routine) (void *), void *arg);
extern int
_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
extern int _pthread_mutex_trylock(pthread_mutex_t *mutex);
extern int __pthread_mutex_lock(pthread_mutex_t *mutex);
extern int _pthread_mutex_unlock(pthread_mutex_t *mutex);
extern int _pthread_mutex_destroy(pthread_mutex_t *mutex);
extern int _pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

extern void LogDone();

typedef struct ThreadState {
    void	*(*start)(void *);
    void	*arg;
} ThreadState;

static ThreadState threadState[RRLOG_MAX_THREADS];

void *
thrwrapper(void *arg)
{
    uintptr_t thrNo = (uintptr_t)arg;

    threadId = thrNo;

    return (threadState[thrNo].start)(threadState[thrNo].arg);
}

/*
 * pthread_create can cause log entries to be out of order because of thread 
 * creation.  We should add the go code that prevents log entries in the middle 
 * of the current log entry.
 */
int
pthread_create(pthread_t * thread, const pthread_attr_t * attr,
	       void *(*start_routine) (void *), void *arg)
{
    int result;
    uintptr_t thrNo;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return _pthread_create(thread, attr, start_routine, arg);
    }

    if (rrMode == RRMODE_RECORD) {
	thrNo = RRShared_AllocThread(rrlog);

	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_THREAD_CREATE;
	e->value[1] = thrNo;
	RRLog_Append(rrlog, e);

	threadState[thrNo].start = start_routine;
	threadState[thrNo].arg = arg;

	result = _pthread_create(thread, attr, thrwrapper, (void *)thrNo);
	ASSERT_IMPLEMENTED(result == 0);
    } else {
	e = RRPlay_Dequeue(rrlog, threadId);
	thrNo = e->value[1];
	AssertEvent(e, RREVENT_THREAD_CREATE);
	RRPlay_Free(rrlog, e);

	RRShared_SetupThread(rrlog, thrNo);

	threadState[thrNo].start = start_routine;
	threadState[thrNo].arg = arg;

	result = _pthread_create(thread, attr, thrwrapper, (void *)thrNo);
	ASSERT_IMPLEMENTED(result == 0);
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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_MUTEX_INIT;
	    e->objectId = (uint64_t)mtx;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_MUTEX_INIT);
	    RRPlay_Free(rrlog, e);
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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_MUTEX_DESTROY;
	    e->objectId = (uint64_t)mtx;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_MUTEX_DESTROY);
	    RRPlay_Free(rrlog, e);
	    break;
    }

    return _pthread_mutex_destroy(mtx);
}

int
_pthread_mutex_lock(pthread_mutex_t *mtx)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = __pthread_mutex_lock(mtx);
	    break;
	}
	case RRMODE_RECORD: {
	    RRLog_LEnter(threadId, (uint64_t)mtx);

	    result = __pthread_mutex_lock(mtx);

	    e = RRLog_LAlloc(threadId);
	    e->event = RREVENT_MUTEX_LOCK;
	    e->objectId = (uint64_t)mtx;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_LAppend(e);
	    break;
	}
	case RRMODE_REPLAY: {
	    RRPlay_LEnter(threadId, (uint64_t)mtx);

	    result = __pthread_mutex_lock(mtx);

	    e = RRPlay_LDequeue(threadId);
	    AssertEvent(e, RREVENT_MUTEX_LOCK);
	    // ASSERT result = e->value[0];
	    RRPlay_LFree(e);
	    break;
	}
	/*
	 * case RRMODE_FDREPLAY: {
	 *     e = RRLog_Alloc(rrlog, threadId);
	 *     e->objectId = 1;
	 *     result = _pthread_mutex_lock(mtx);
	 *     RRLog_Append(rrlog, e);
	 *     break;
	 * }
	 * case RRMODE_FTREPLAY: {
	 *     e = RRPlay_Dequeue(rrlog, threadId);
	 *     result = _pthread_mutex_lock(mtx);
	 *     RRPlay_Free(rrlog, e);
	 *     break;
	 * }
	 */
    }

    return result;
}

int
pthread_mutex_trylock(pthread_mutex_t *mtx)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_mutex_trylock(mtx);
	    break;
	}
	case RRMODE_RECORD: {
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_MUTEX_TRYLOCK;
	    e->objectId = (uint64_t)mtx;
	    result = _pthread_mutex_trylock(mtx);
	    e->value[0] = (uint64_t)result;
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    e = RRPlay_Dequeue(rrlog, threadId);
	    result = (int)e->value[0];
	    RRPlay_Free(rrlog, e);
	    break;
	}
	/*
	 * case RRMODE_FDREPLAY: {
	 *     e = RRLog_Alloc(rrlog, threadId);
	 *     e->objectId = 1;
	 *     result = _pthread_mutex_trylock(mtx);
	 *     RRLog_Append(rrlog, e);
	 *     break;
	 * }
	 * case RRMODE_FTREPLAY: {
	 *     e = RRPlay_Dequeue(rrlog, threadId);
	 *     result = _pthread_mutex_trylock(mtx);
	 *     RRPlay_Free(rrlog, e);
	 *     break;
	 * }
	 */
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
	    result = _pthread_mutex_unlock(mtx);
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_MUTEX_UNLOCK;
	    e->objectId = (uint64_t)mtx;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    result = _pthread_mutex_unlock(mtx);
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_MUTEX_UNLOCK);
	    // ASSERT result = e->value[0];
	    RRPlay_Free(rrlog, e);
	    break;
	}
	/*
	 * case RRMODE_FDREPLAY: {
	 *     result = _pthread_mutex_unlock(mtx);
	 *     break;
	 * }
	 * case RRMODE_FTREPLAY: {
	 *     result = _pthread_mutex_unlock(mtx);
	 *     break;
	 * }
	 */
    }

    return result;
}

int
pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_once(once_control, init_routine);
	    break;
	}
	case RRMODE_RECORD: {
	    RRLog_LEnter(threadId, (uint64_t)once_control);

	    result = _pthread_once(once_control, init_routine);

	    e = RRLog_LAlloc(threadId);
	    e->event = RREVENT_THREAD_ONCE;
	    e->objectId = (uint64_t)once_control;
	    e->value[0] = (uint64_t)result;
	    RRLog_LAppend(e);
	    break;
	}
	case RRMODE_REPLAY: {
	    RRPlay_LEnter(threadId, (uint64_t)once_control);

	    result = _pthread_once(once_control, init_routine);

	    e = RRPlay_LDequeue(threadId);
	    AssertEvent(e, RREVENT_THREAD_ONCE);
	    // ASSERT result = e->value[0];
	    RRPlay_LFree(e);
	    break;
	}
    }

    return result;
}

pid_t
__libc_fork(void)
{
	return (((pid_t (*)(void))__libc_interposing[INTERPOS_fork])());
}

pid_t
__rr_fork(void)
{
    pid_t result;
    uintptr_t thrNo;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return __libc_fork();
    }

    if (rrMode == RRMODE_RECORD) {
	thrNo = RRShared_AllocThread(rrlog);

	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_FORK;
	e->value[1] = thrNo;
	RRLog_Append(rrlog, e);

	result = __libc_fork();

	if (result == 0) {
	    threadId = thrNo;
	} else {
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_FORKEND;
	    e->value[0] = (uint64_t)result;
	    if (result == -1) {
		e->value[1] = (uint64_t)errno;
	    }
	    RRLog_Append(rrlog, e);
	}
    } else {
	e = RRPlay_Dequeue(rrlog, threadId);
	AssertEvent(e, RREVENT_FORK);
	thrNo = e->value[1];
	RRPlay_Free(rrlog, e);

	RRShared_SetupThread(rrlog, thrNo);

	int rstatus = __libc_fork();
	if (rstatus < 0) {
	    PERROR("Unable to fork on replay!");
	}

	if (rstatus != 0) {
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_FORKEND);
	    result = (int)e->value[0];
	    if (result == -1) {
		errno = (int)e->value[1];
	    }
	    RRPlay_Free(rrlog, e);
	} else {
	    threadId = thrNo;
	    result = 0;
	}
    }

    return result;
}

void
__rr_exit(int status)
{
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	syscall(SYS_exit, status);
	__builtin_unreachable();
    }

    if (rrMode == RRMODE_RECORD) {
	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_EXIT;
	RRLog_Append(rrlog, e);
    } else {
	e = RRPlay_Dequeue(rrlog, threadId);
	AssertEvent(e, RREVENT_EXIT);
	RRPlay_Free(rrlog, e);
    }

    // Drain log
    LogDone();

    syscall(SYS_exit, status);
}

pid_t
__rr_getpid()
{
    pid_t pid;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return __sys_getpid();
    }

    if (rrMode == RRMODE_RECORD) {
	pid = __sys_getpid();
	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_GETPID;
	e->value[0] = (uint64_t)pid;
	RRLog_Append(rrlog, e);
    } else {
	e = RRPlay_Dequeue(rrlog, threadId);
	AssertEvent(e, RREVENT_GETPID);	
	pid = (pid_t)e->value[0];
	RRPlay_Free(rrlog, e);
    }

    return pid;
}

pid_t
__rr_getppid()
{
    pid_t pid;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return __sys_getppid();
    }

    if (rrMode == RRMODE_RECORD) {
	pid = __sys_getppid();
	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_GETPPID;
	e->value[0] = (uint64_t)pid;
	RRLog_Append(rrlog, e);
    } else {
	e = RRPlay_Dequeue(rrlog, threadId);
	AssertEvent(e, RREVENT_GETPPID);	
	pid = (pid_t)e->value[0];
	RRPlay_Free(rrlog, e);
    }

    return pid;
}

/*
 * Will remove this function and replace is it with wait4/6 through the 
 * interposing table once pidmap is checked in.
 */
pid_t
__rr_wait(int *status)
{
    pid_t pid;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return __sys_wait4(WAIT_ANY, status, 0, NULL);
    }

    if (rrMode == RRMODE_RECORD) {
	pid = __sys_wait4(WAIT_ANY, status, 0, NULL);
	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_WAIT;
	e->value[0] = (uint64_t)pid;
	if (pid == -1) {
	    e->value[1] = (uint64_t)errno;
	}
	if (status != NULL) {
	    e->value[2] = (uint64_t)*status;
	}
	RRLog_Append(rrlog, e);
    } else {
	// XXX: Use waitpid to wait for the correct child
	__sys_wait4(WAIT_ANY, NULL, 0, NULL);
	e = RRPlay_Dequeue(rrlog, threadId);
	AssertEvent(e, RREVENT_WAIT);	
	pid = (int)e->value[0];
	if (pid == -1) {
	    errno = (int)e->value[1];
	}
	if (status != NULL) {
	    *status = (int)e->value[2];
	}
	RRPlay_Free(rrlog, e);
    }

    return pid;
}

__strong_reference(__rr_exit, _exit);
__strong_reference(_pthread_mutex_lock, pthread_mutex_lock);

BIND_REF(getpid);
BIND_REF(getppid);
BIND_REF(wait);
BIND_REF(fork);

