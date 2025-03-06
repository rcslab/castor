
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
#include <castor/rrshared.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "../Runtime/util.h"

extern int _pthread_create(pthread_t * thread, const pthread_attr_t * attr,
	       void *(*start_routine) (void *), void *arg);
extern int _pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

extern int __pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
extern int __pthread_mutex_trylock(pthread_mutex_t *mutex);
extern int __pthread_mutex_lock(pthread_mutex_t *mutex);
extern int __pthread_mutex_unlock(pthread_mutex_t *mutex);
extern int __pthread_mutex_destroy(pthread_mutex_t *mutex);
extern int _pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abs_timeout);

extern int _pthread_spin_init(pthread_spinlock_t *lock, int pshared);
extern int _pthread_spin_destroy(pthread_spinlock_t *lock);
extern int _pthread_spin_trylock(pthread_spinlock_t *lock);
extern int _pthread_spin_lock(pthread_spinlock_t *lock);
extern int _pthread_spin_unlock(pthread_spinlock_t *lock);

extern int _pthread_rwlock_init(pthread_rwlock_t *lock, const pthread_rwlockattr_t *attr);
extern int _pthread_rwlock_destroy(pthread_rwlock_t *lock);
extern int _pthread_rwlock_unlock(pthread_rwlock_t *lock);
extern int _pthread_rwlock_tryrdlock(pthread_rwlock_t *lock);
extern int _pthread_rwlock_rdlock(pthread_rwlock_t *lock);
extern int _pthread_rwlock_timedrdlock(pthread_rwlock_t *lock, const struct timespec *abs_timeout);
extern int _pthread_rwlock_trywrlock(pthread_rwlock_t *lock);
extern int _pthread_rwlock_wrlock(pthread_rwlock_t *lock);
extern int _pthread_rwlock_timedwrlock(pthread_rwlock_t *lock, const struct timespec *abs_timeout);

extern int _pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
extern int _pthread_cond_destroy(pthread_cond_t *cond);
extern int _pthread_cond_broadcast(pthread_cond_t *cond);
extern int _pthread_cond_signal(pthread_cond_t *cond);
extern int _pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *mutex);

extern int _pthread_barrier_destroy(pthread_barrier_t *barrier);
extern int _pthread_barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned count);
extern int _pthread_barrier_wait(pthread_barrier_t *barrier);

extern int _pthread_detach(pthread_t thread);
extern int _pthread_join(pthread_t thread, void **value_ptr);
extern int _pthread_timedjoin_np(pthread_t thread, void **value_ptr,
         const struct timespec *abstime);
extern int _pthread_kill(pthread_t thread, int sig);

typedef struct ThreadState {
    void	*(*start)(void *);
    void	*arg;
} ThreadState;

struct ProcThreadInfo {
    uint64_t recordPid;
    uint64_t thrNo;
};

static ThreadState threadState[RRLOG_MAX_THREADS];

void *
thrwrapper(void *arg)
{
    struct ProcThreadInfo *ptInfo = (struct ProcThreadInfo *)arg;
    //uintptr_t thrNo = (uintptr_t)arg;
    uintptr_t thrNo = ptInfo->thrNo;

    setThreadId(thrNo, ptInfo->recordPid);
    free(ptInfo);

    return (threadState[thrNo].start)(threadState[thrNo].arg);
}

/*
 * XXX: pthread_create can cause log entries to be out of order because of thread 
 * creation.  We should add the go code that prevents log entries in the middle 
 * of the current log entry.
 */
int
pthread_create(pthread_t * thread, const pthread_attr_t * attr,
	       void *(*start_routine) (void *), void *arg)
{
    int result;
    uintptr_t thrNo;
    struct ProcThreadInfo *ptInfo;
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return _pthread_create(thread, attr, start_routine, arg);
    }

    ptInfo = malloc(sizeof(struct ProcThreadInfo));
    if (rrMode == RRMODE_RECORD) {
	e = RRLog_Alloc(rrlog, getThreadId());
	e->event = RREVENT_THREAD_CREATE;
	thrNo = RRShared_AllocThread(rrlog);
	ptInfo->recordPid = 0;
	ptInfo->thrNo = thrNo;
	e->value[1] = thrNo;
	RRLog_Append(rrlog, e);
    } else {
	e = RRPlay_Dequeue(rrlog, getThreadId());
	thrNo = e->value[1];
	ptInfo->recordPid = getRecordedPid();
	ptInfo->thrNo = thrNo;
	AssertEvent(e, RREVENT_THREAD_CREATE);
	RRPlay_Free(rrlog, e);
	RRShared_SetupThread(rrlog, thrNo);
    }

    threadState[thrNo].start = start_routine;
    threadState[thrNo].arg = arg;
    result = _pthread_create(thread, attr, thrwrapper, (void *)ptInfo);
    ASSERT_IMPLEMENTED(result == 0);
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
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_COND_INIT;
	    e->objectId = (uint64_t)cond;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, getThreadId());
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
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_COND_DESTROY;
	    e->objectId = (uint64_t)cond;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, getThreadId());
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
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_COND_WAIT;
	    e->objectId = (uint64_t)cond;
	    RRLog_Append(rrlog, e);
      
	    result = _pthread_cond_wait(cond, mutex);

	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)mutex, RRSync_Mutex);

	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_MUTEX_LOCK;
	    e->objectId = (uint64_t)mutex;
	    e->value[0] = (uint64_t)result;
	    e->value[3] = atomic_load(&s->epoch);
	    e->value[4] = __builtin_readcyclecounter(); //XXX: where is this used?
	    RRLog_Append(rrlog, e);

	    if (result == 0) {
		RRShared_SyncInc(s);
	    }

	    break;
	}
	case RRMODE_REPLAY: {
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_COND_WAIT);
	    RRPlay_Free(rrlog, e);

	    int tmpresult = __pthread_mutex_unlock(mutex);
	    ASSERT(tmpresult == 0);

	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)mutex, RRSync_Mutex);

	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_MUTEX_LOCK);
	    result = e->value[0];
	    uint64_t epoch = e->value[3];
	    RRPlay_Free(rrlog, e);
	    if (result != 0) {
		return result;
	    }

	    RRShared_SyncWait(s, epoch);

	    result = __pthread_mutex_lock(mutex);
	    ASSERT(result == 0);
	    RRShared_SyncInc(s);

	    break;
	}
    }

    return result;
}

int
pthread_cond_signal(pthread_cond_t *cond)
{
    int cs_result, result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_cond_signal(cond);
	    break;
	}
	case RRMODE_RECORD: {
	    result = _pthread_cond_signal(cond);
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_COND_SIGNAL;
	    e->objectId = (uint64_t)cond;
	    e->value[0] = (uint64_t)result;
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    cs_result = _pthread_cond_signal(cond);
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_COND_SIGNAL);
	    result = (int)e->value[0];
	    ASSERT(cs_result == result);
	    RRPlay_Free(rrlog, e);
	    break;
	}
    }

    return result;
}

int
pthread_cond_broadcast(pthread_cond_t *cond)
{
    int cb_result, result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_cond_broadcast(cond);
	    break;
	}
	case RRMODE_RECORD: {
	    result = _pthread_cond_broadcast(cond);
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_COND_BROADCAST;
	    e->objectId = (uint64_t)cond;
	    e->value[0] = (uint64_t)result;
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    cb_result = _pthread_cond_broadcast(cond);
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_COND_BROADCAST);
	    result = (int)e->value[0];
	    ASSERT(cb_result == result);
	    RRPlay_Free(rrlog, e);
	    break;
	}
    }

    return result;
}

int
pthread_barrier_destroy(pthread_barrier_t *barrier)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_BARRIER_DESTROY;
	    e->objectId = (uint64_t)barrier;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_BARRIER_DESTROY);
	    RRPlay_Free(rrlog, e);
	    break;
    }

    return _pthread_barrier_destroy(barrier);
}

int
pthread_barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned count)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_BARRIER_INIT;
	    e->objectId = (uint64_t)barrier;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_BARRIER_INIT);
	    RRPlay_Free(rrlog, e);
	    break;
    }
    return _pthread_barrier_init(barrier, attr, count);
}

int
pthread_barrier_wait(pthread_barrier_t *barrier)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_barrier_wait(barrier);
	    break;
	}
	case RRMODE_RECORD: {
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_BARRIER_WAIT;
	    e->objectId = (uint64_t)barrier;
	    RRLog_Append(rrlog, e);
	    result = _pthread_barrier_wait(barrier);
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_BARRIER_WAIT;
	    e->objectId = (uint64_t)barrier;
	    e->value[0] = (uint64_t)result;
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_BARRIER_WAIT);
	    RRPlay_Free(rrlog, e);
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    result = (int)e->value[0];
	    AssertEvent(e, RREVENT_BARRIER_WAIT);
      RRPlay_Free(rrlog, e);
	    break;
	}
    }

    return result;
}

int
__rr_mutex_init(pthread_mutex_t *mtx, const pthread_mutexattr_t *attr)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_MUTEX_INIT;
	    e->objectId = (uint64_t)mtx;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_MUTEX_INIT);
	    RRPlay_Free(rrlog, e);
	    break;
    }
    return __pthread_mutex_init(mtx, attr);
}

int
__rr_mutex_destroy(pthread_mutex_t *mtx)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_MUTEX_DESTROY;
	    e->objectId = (uint64_t)mtx;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_MUTEX_DESTROY);
	    RRPlay_Free(rrlog, e);
	    break;
    }

    return __pthread_mutex_destroy(mtx);
}

int
__rr_mutex_lock(pthread_mutex_t *mtx)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = __pthread_mutex_lock(mtx);
	    break;
	}
	case RRMODE_RECORD: {
	    result = __pthread_mutex_lock(mtx);

	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)mtx, RRSync_Mutex);

	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_MUTEX_LOCK;
	    e->objectId = (uint64_t)mtx;
	    e->value[0] = (uint64_t)result;
	    e->value[3] = atomic_load(&s->epoch);
	    e->value[4] = __builtin_readcyclecounter(); //XXX: where is this used?
	    RRLog_Append(rrlog, e);

	    if (result == 0) {
		RRShared_SyncInc(s);
	    }

	    break;
	}
	case RRMODE_REPLAY: {
	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)mtx, RRSync_Mutex);

	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_MUTEX_LOCK);
	    result = e->value[0];
	    uint64_t epoch = e->value[3];
	    RRPlay_Free(rrlog, e);
	    if (result != 0) {
		return result;
	    }

	    RRShared_SyncWait(s, epoch);

	    result = __pthread_mutex_lock(mtx);
	    ASSERT(result == 0);
	    RRShared_SyncInc(s);

	    break;
	}
    }

    return result;
}

int
__rr_mutex_trylock(pthread_mutex_t *mtx)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = __pthread_mutex_trylock(mtx);
	    break;
	}
	case RRMODE_RECORD: {
	    result = __pthread_mutex_lock(mtx);

	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)mtx, RRSync_Mutex);

	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_MUTEX_TRYLOCK;
	    e->objectId = (uint64_t)mtx;
	    e->value[0] = (uint64_t)result;
	    e->value[3] = atomic_load(&s->epoch);
	    e->value[4] = __builtin_readcyclecounter(); //XXX: where is this used?
	    RRLog_Append(rrlog, e);

	    if (result == 0) {
		RRShared_SyncInc(s);
	    }

	    break;
	}
	case RRMODE_REPLAY: {
	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)mtx, RRSync_Mutex);

	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_MUTEX_TRYLOCK);
	    uint64_t epoch = e->value[3];
	    result = e->value[0];
	    RRPlay_Free(rrlog, e);
	    if (result != 0) {
		return result;
	    }

	    RRShared_SyncWait(s, epoch);

	    result = __pthread_mutex_lock(mtx);
	    ASSERT(result == 0);
	    RRShared_SyncInc(s);

	    break;
	}
    }

    return result;
}

int
__rr_mutex_unlock(pthread_mutex_t *mtx)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = __pthread_mutex_unlock(mtx);
	    break;
	}
	case RRMODE_RECORD: {
	    result = __pthread_mutex_unlock(mtx);
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_MUTEX_UNLOCK;
	    e->objectId = (uint64_t)mtx;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    result = __pthread_mutex_unlock(mtx);
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_MUTEX_UNLOCK);
	    ASSERT(result == (int)e->value[0]);
	    RRPlay_Free(rrlog, e);
	    break;
	}
    }

    return result;
}

int 
pthread_mutex_timedlock(pthread_mutex_t *mtx, const struct timespec *abs_timeout)
{
    int result = -1;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_mutex_timedlock(mtx, abs_timeout);
	    break;
	}
	case RRMODE_RECORD: {
	    result = __pthread_mutex_lock(mtx);

	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)mtx, RRSync_Mutex);

	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_MUTEX_TIMEDLOCK;
	    e->objectId = (uint64_t)mtx;
	    e->value[0] = (uint64_t)result;
	    e->value[3] = atomic_load(&s->epoch);
	    e->value[4] = __builtin_readcyclecounter(); //XXX: where is this used?
	    RRLog_Append(rrlog, e);

	    if (result == 0) {
		RRShared_SyncInc(s);
	    }

	    break;
	}
	case RRMODE_REPLAY: {
	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)mtx, RRSync_Mutex);

	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_MUTEX_TIMEDLOCK);
	    result = e->value[0];
	    uint64_t epoch = e->value[3];
	    RRPlay_Free(rrlog, e);
	    if (result != 0) {
		return result;
	    }

	    RRShared_SyncWait(s, epoch);

	    do {
		result = _pthread_mutex_timedlock(mtx, abs_timeout);
	    } while (result != 0);
	    ASSERT(result == 0);
	    RRShared_SyncInc(s);

	    break;
	}
    }

    return result;
}

int
pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_SPIN_INIT;
	    e->objectId = (uint64_t)lock;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_SPIN_INIT);
	    RRPlay_Free(rrlog, e);
	    break;
    }
    return _pthread_spin_init(lock, pshared);
}

int
pthread_spin_destroy(pthread_spinlock_t *lock)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_SPIN_DESTROY;
	    e->objectId = (uint64_t)lock;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_SPIN_DESTROY);
	    RRPlay_Free(rrlog, e);
	    break;
    }

    return _pthread_spin_destroy(lock);
}

int
pthread_spin_lock(pthread_spinlock_t *lock)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_spin_lock(lock);
	    break;
	}
	case RRMODE_RECORD: {
	    result = _pthread_spin_lock(lock);

	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)lock, RRSync_Spinlock);

	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_SPIN_LOCK;
	    e->objectId = (uint64_t)lock;
	    e->value[0] = (uint64_t)result;
	    e->value[3] = atomic_load(&s->epoch);
	    e->value[4] = __builtin_readcyclecounter(); //XXX: where is this used?
	    RRLog_Append(rrlog, e);

	    if (result == 0) {
		RRShared_SyncInc(s);
	    }

	    break;
	}
	case RRMODE_REPLAY: {
	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)lock, RRSync_Spinlock);

	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_SPIN_LOCK);
	    result = e->value[0];
	    uint64_t epoch = e->value[3];
	    RRPlay_Free(rrlog, e);
	    if (result != 0) {
		return result;
	    }

	    RRShared_SyncWait(s, epoch);

	    result = _pthread_spin_lock(lock);
	    ASSERT(result == 0);
	    RRShared_SyncInc(s);

	    break;
	}
    }

    return result;
}

int
pthread_spin_trylock(pthread_spinlock_t *lock)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_spin_trylock(lock);
	    break;
	}
	case RRMODE_RECORD: {
	    result = _pthread_spin_lock(lock);

	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)lock, RRSync_Spinlock);

	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_SPIN_TRYLOCK;
	    e->objectId = (uint64_t)lock;
	    e->value[0] = (uint64_t)result;
	    e->value[3] = atomic_load(&s->epoch);
	    e->value[4] = __builtin_readcyclecounter(); //XXX: where is this used?
	    RRLog_Append(rrlog, e);

	    if (result == 0) {
		RRShared_SyncInc(s);
	    }
	    break;
	}
	case RRMODE_REPLAY: {
	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)lock, RRSync_Spinlock);

	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_SPIN_TRYLOCK);
	    result = e->value[0];
	    uint64_t epoch = e->value[3];
	    RRPlay_Free(rrlog, e);
	    if (result != 0) {
		return result;
	    }

	    RRShared_SyncWait(s, epoch);

	    result = _pthread_spin_lock(lock);
	    ASSERT(result == 0);
	    RRShared_SyncInc(s);
	    break;
	}
  }

    return result;
}

int
pthread_spin_unlock(pthread_spinlock_t *lock)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_spin_unlock(lock);
	    break;
	}
	case RRMODE_RECORD: {
	    result = _pthread_spin_unlock(lock);
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_SPIN_UNLOCK;
	    e->objectId = (uint64_t)lock;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    result = _pthread_spin_unlock(lock);
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_SPIN_UNLOCK);
	    // ASSERT result = e->value[0];
	    RRPlay_Free(rrlog, e);
	    break;
	}
    }

    return result;
}

int
pthread_rwlock_init(pthread_rwlock_t *lock, const pthread_rwlockattr_t *attr)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_RWLOCK_INIT;
	    e->objectId = (uint64_t)lock;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_RWLOCK_INIT);
	    RRPlay_Free(rrlog, e);
	    break;
    }
    return _pthread_rwlock_init(lock, attr);
}

int
pthread_rwlock_destroy(pthread_rwlock_t *lock)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_RWLOCK_DESTROY;
	    e->objectId = (uint64_t)lock;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_RWLOCK_DESTROY);
	    RRPlay_Free(rrlog, e);
	    break;
    }

    return _pthread_rwlock_destroy(lock);
}

int
pthread_rwlock_rdlock(pthread_rwlock_t *lock)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_rwlock_rdlock(lock);
	    break;
	}
	case RRMODE_RECORD: {
	    result = _pthread_rwlock_rdlock(lock);

	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)lock, RRSync_RWLock);

	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_RWLOCK_RDLOCK;
	    e->objectId = (uint64_t)lock;
	    e->value[0] = (uint64_t)result;
	    e->value[3] = atomic_load(&s->epoch);
	    e->value[4] = __builtin_readcyclecounter(); //XXX: where is this used?
	    RRLog_Append(rrlog, e);

	    if (result == 0) {
		RRShared_SyncInc(s);
	    }

	    break;
	}
	case RRMODE_REPLAY: {
	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)lock, RRSync_RWLock);

	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_RWLOCK_RDLOCK);
	    result = e->value[0];
	    uint64_t epoch = e->value[3];
	    RRPlay_Free(rrlog, e);
	    if (result != 0) {
		return result;
	    }

	    RRShared_SyncWait(s, epoch);

	    result = _pthread_rwlock_rdlock(lock);
	    ASSERT(result == 0);
	    RRShared_SyncInc(s);
	    break;
	}
    }

    return result;
}

int
pthread_rwlock_timedrdlock(pthread_rwlock_t *lock, const struct timespec *abs_timeout)
{
    int result = -1;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_rwlock_timedrdlock(lock, abs_timeout);
	    break;
	}
	case RRMODE_RECORD: {
	    RRLog_LEnter(getThreadId(), (uint64_t)lock);

	    result = _pthread_rwlock_timedrdlock(lock, abs_timeout);

	    e = RRLog_LAlloc(getThreadId());
	    e->event = RREVENT_RWLOCK_TIMEDRDLOCK;
	    e->objectId = (uint64_t)lock;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_LAppend(e);
	    ASSERT(false);
	    break;
	}
	case RRMODE_REPLAY: {
	    RRPlay_LEnter(getThreadId(), (uint64_t)lock);
	    e = RRPlay_LDequeue(getThreadId());
	    AssertEvent(e, RREVENT_RWLOCK_TIMEDRDLOCK);

	    ASSERT(false);
	    /* Record call was successful, wait until replay call matches */
	    if (e->value[0] == 0) {
		while (result != 0) {
		    result = _pthread_rwlock_timedrdlock(lock, abs_timeout);
		}
	    } else {
		result = e->value[0];
	    }

	    RRPlay_LFree(e);
	    break;
	}
    }

    return result;
}

int
pthread_rwlock_tryrdlock(pthread_rwlock_t *lock)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_rwlock_tryrdlock(lock);
	    break;
	}
	case RRMODE_RECORD: {
	    ASSERT(false);
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_RWLOCK_TRYRDLOCK;
	    e->objectId = (uint64_t)lock;
	    result = _pthread_rwlock_tryrdlock(lock);
	    e->value[0] = (uint64_t)result;
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    ASSERT(false);
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_RWLOCK_TRYRDLOCK);
	    result = (int)e->value[0];
	    RRPlay_Free(rrlog, e);
	    break;
	}
  }

    return result;
}

int
pthread_rwlock_wrlock(pthread_rwlock_t *lock)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_rwlock_wrlock(lock);
	    break;
	}
	case RRMODE_RECORD: {
	    result = _pthread_rwlock_wrlock(lock);

	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)lock, RRSync_RWLock);

	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_RWLOCK_WRLOCK;
	    e->objectId = (uint64_t)lock;
	    e->value[0] = (uint64_t)result;
	    e->value[3] = atomic_load(&s->epoch);
	    e->value[4] = __builtin_readcyclecounter(); //XXX: where is this used?
	    RRLog_Append(rrlog, e);

	    if (result == 0) {
		RRShared_SyncInc(s);
	    }

	    break;
	}
	case RRMODE_REPLAY: {
	    RRSyncEntry *s = RRShared_SyncLookup(rrlog, (uintptr_t)lock, RRSync_RWLock);

	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_RWLOCK_WRLOCK);
	    result = e->value[0];
	    uint64_t epoch = e->value[3];
	    RRPlay_Free(rrlog, e);
	    if (result != 0) {
		return result;
	    }

	    RRShared_SyncWait(s, epoch);

	    result = _pthread_rwlock_wrlock(lock);
	    ASSERT(result == 0);
	    RRShared_SyncInc(s);
	    break;
	}
    }

    return result;
}

int
pthread_rwlock_timedwrlock(pthread_rwlock_t *lock, const struct timespec *abs_timeout)
{
    int result = -1;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_rwlock_timedwrlock(lock, abs_timeout);
	    break;
	}
	case RRMODE_RECORD: {
	    RRLog_LEnter(getThreadId(), (uint64_t)lock);

	    result = _pthread_rwlock_timedwrlock(lock, abs_timeout);

	    e = RRLog_LAlloc(getThreadId());
	    e->event = RREVENT_RWLOCK_TIMEDWRLOCK;
	    e->objectId = (uint64_t)lock;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_LAppend(e);
	    ASSERT(false);
	    break;
	}
	case RRMODE_REPLAY: {
	    RRPlay_LEnter(getThreadId(), (uint64_t)lock);
	    e = RRPlay_LDequeue(getThreadId());
	    AssertEvent(e, RREVENT_RWLOCK_TIMEDWRLOCK);

	    ASSERT(false);
	    /* Record call was successful, wait until replay call matches */
	    if (e->value[0] == 0) {
		while (result != 0) {
		    result = _pthread_rwlock_timedwrlock(lock, abs_timeout);
		}
	    } else {
		result = e->value[0];
	    }

	    RRPlay_LFree(e);
	    break;
	}
    }

    return result;
}

int
pthread_rwlock_trywrlock(pthread_rwlock_t *lock)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_rwlock_trywrlock(lock);
	    break;
	}
	case RRMODE_RECORD: {
	    ASSERT(false);
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_RWLOCK_TRYWRLOCK;
	    e->objectId = (uint64_t)lock;
	    result = _pthread_rwlock_trywrlock(lock);
	    e->value[0] = (uint64_t)result;
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    ASSERT(false);
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_RWLOCK_TRYWRLOCK);
	    result = (int)e->value[0];
	    RRPlay_Free(rrlog, e);
	    break;
	}
  }

    return result;
}

int
pthread_rwlock_unlock(pthread_rwlock_t *lock)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_rwlock_unlock(lock);
	    break;
	}
	case RRMODE_RECORD: {
	    result = _pthread_rwlock_unlock(lock);
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_RWLOCK_UNLOCK;
	    e->objectId = (uint64_t)lock;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    result = _pthread_rwlock_unlock(lock);
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_RWLOCK_UNLOCK);
	    // ASSERT result = e->value[0];
	    RRPlay_Free(rrlog, e);
	    break;
	}
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
	    RRLog_LEnter(getThreadId(), (uint64_t)once_control);
	    result = _pthread_once(once_control, init_routine);
	    e = RRLog_LAlloc(getThreadId());
	    e->event = RREVENT_THREAD_ONCE;
	    e->objectId = (uint64_t)once_control;
	    e->value[0] = (uint64_t)result;
	    RRLog_LAppend(e);
	    break;
	}
	case RRMODE_REPLAY: {
	    RRPlay_LEnter(getThreadId(), (uint64_t)once_control);

	    result = _pthread_once(once_control, init_routine);

	    e = RRPlay_LDequeue(getThreadId());
	    AssertEvent(e, RREVENT_THREAD_ONCE);
	    // ASSERT result = e->value[0];
	    RRPlay_LFree(e);
	    break;
	}
    }

    return result;
}

int
pthread_detach(pthread_t thread)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_detach(thread);
	    break;
	}
	case RRMODE_RECORD: {
	    result = _pthread_detach(thread);
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_THREAD_DETACH;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    result = _pthread_detach(thread);
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_THREAD_DETACH);
	    RRPlay_Free(rrlog, e);
	    break;
	}
    }

    return result;
}

int
pthread_join(pthread_t thread, void **value_ptr)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_join(thread, value_ptr);
	    break;
	}
	case RRMODE_RECORD: {
	    result = _pthread_join(thread, value_ptr);
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_THREAD_JOIN;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    result = _pthread_join(thread, value_ptr);
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_THREAD_JOIN);
	    RRPlay_Free(rrlog, e);
	    break;
	}
    }

    return result;
}

int
pthread_timedjoin_np(pthread_t thread, void **value_ptr,
         const struct timespec *abstime)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_timedjoin_np(thread, value_ptr, abstime);
	    break;
	}
	case RRMODE_RECORD: {
	    RRLog_LEnter(getThreadId(), (uint64_t)thread);
	    result = _pthread_timedjoin_np(thread, value_ptr, abstime);
	    e = RRLog_LAlloc(getThreadId());
	    e->event = RREVENT_THREAD_TIMEDJOIN;
	    e->objectId = (uint64_t)thread;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_LAppend(e);
	    break;
	}
	case RRMODE_REPLAY: {
	    RRPlay_LEnter(getThreadId(), (uint64_t)thread);

	    /* Dequeue until we reaches the TIMEDJOIN event*/
	    while (true) {
		e = RRPlay_Dequeue(rrlog, getThreadId());
		if (e->event == RREVENT_THREAD_TIMEDJOIN)
		    break; 
		/* We should only see CLCOK_GETTIME event */
		AssertEvent(e, RREVENT_CLOCK_GETTIME);
		RRPlay_Free(rrlog, e);
	    }

	    if (e->value[0] == 0) {
		_pthread_join(thread, value_ptr);
	    }

	    result = e->value[0];
	    RRPlay_LFree(e);
	    break;
	}
    }

    return result;
}

/* Does not ensure that signals are received
   in the same order. */
int
pthread_kill(pthread_t thread, int sig)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_kill(thread, sig);
	    break;
	}
	case RRMODE_RECORD: {
	    result = _pthread_kill(thread, sig);
	    e = RRLog_Alloc(rrlog, getThreadId());
	    e->event = RREVENT_THREAD_KILL;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter(); //XXX: why is this here
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    result = _pthread_kill(thread, sig);
	    e = RRPlay_Dequeue(rrlog, getThreadId());
	    AssertEvent(e, RREVENT_THREAD_KILL);
	    RRPlay_Free(rrlog, e);
	    break;
	}
    }

    return result;
}

__strong_reference(__rr_mutex_lock, pthread_mutex_lock);
__strong_reference(__rr_mutex_lock, _pthread_mutex_lock);
__strong_reference(__rr_mutex_unlock, pthread_mutex_unlock);
__strong_reference(__rr_mutex_unlock, _pthread_mutex_unlock);
__strong_reference(__rr_mutex_trylock, pthread_mutex_trylock);
__strong_reference(__rr_mutex_trylock, _pthread_mutex_trylock);
