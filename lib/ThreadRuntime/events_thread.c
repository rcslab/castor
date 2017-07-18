
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

#include "../Runtime/util.h"

extern int _pthread_create(pthread_t * thread, const pthread_attr_t * attr,
	       void *(*start_routine) (void *), void *arg);

extern int _pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
extern int _pthread_mutex_trylock(pthread_mutex_t *mutex);
extern int __pthread_mutex_lock(pthread_mutex_t *mutex);
extern int _pthread_mutex_unlock(pthread_mutex_t *mutex);
extern int _pthread_mutex_destroy(pthread_mutex_t *mutex);
extern int _pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

extern int _pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
extern int _pthread_cond_destroy(pthread_cond_t *cond);
extern int _pthread_cond_broadcast(pthread_cond_t *cond);
extern int _pthread_cond_signal(pthread_cond_t *cond);
extern int _pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *mutex);

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
pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_COND_INIT;
	    e->objectId = (uint64_t)cond;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_COND_INIT);
	    RRPlay_Free(rrlog, e);
	    break;
    }
    return _pthread_cond_init(cond, attr);
}

int
pthread_cond_destroy(pthread_cond_t *cond)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_COND_DESTROY;
	    e->objectId = (uint64_t)cond;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_COND_DESTROY);
	    RRPlay_Free(rrlog, e);
	    break;
    }
    return _pthread_cond_destroy(cond);
}


int
pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_cond_wait(cond, mutex);
	    break;
	}
	case RRMODE_RECORD: {
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_COND_WAIT;
	    e->objectId = (uint64_t)cond;
	    RRLog_Append(rrlog, e);
	    result = _pthread_cond_wait(cond, mutex);
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_COND_WAIT;
	    e->objectId = (uint64_t)cond;
	    e->value[0] = (uint64_t)result;
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_COND_WAIT);
	    _pthread_mutex_unlock(mutex);
	    RRPlay_Free(rrlog, e);
	    e = RRPlay_Dequeue(rrlog, threadId);
	    result = (int)e->value[0];
	    AssertEvent(e, RREVENT_COND_WAIT);
	    __pthread_mutex_lock(mutex);
	    RRPlay_Free(rrlog, e);
	    break;
	}
    }

    return result;
}

int
pthread_cond_signal(pthread_cond_t *cond)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_cond_signal(cond);
	    break;
	}
	case RRMODE_RECORD: {
	    result = _pthread_cond_signal(cond);
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_COND_SIGNAL;
	    e->objectId = (uint64_t)cond;
	    e->value[0] = (uint64_t)result;
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_COND_SIGNAL);
	    result = (int)e->value[0];
	    RRPlay_Free(rrlog, e);
	    break;
	}
    }

    return result;
}

int
pthread_cond_broadcast(pthread_cond_t *cond)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_cond_broadcast(cond);
	    break;
	}
	case RRMODE_RECORD: {
	    result = _pthread_cond_broadcast(cond);
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_COND_BROADCAST;
	    e->objectId = (uint64_t)cond;
	    e->value[0] = (uint64_t)result;
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_COND_BROADCAST);
	    result = (int)e->value[0];
	    RRPlay_Free(rrlog, e);
	    break;
	}
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

__strong_reference(_pthread_mutex_lock, pthread_mutex_lock);

