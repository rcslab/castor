
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
extern int _pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

extern int _pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
extern int _pthread_mutex_trylock(pthread_mutex_t *mutex);
extern int __pthread_mutex_lock(pthread_mutex_t *mutex);
extern int _pthread_mutex_unlock(pthread_mutex_t *mutex);
extern int _pthread_mutex_destroy(pthread_mutex_t *mutex);
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

static ThreadState threadState[RRLOG_MAX_THREADS];

void *
thrwrapper(void *arg)
{
    uintptr_t thrNo = (uintptr_t)arg;

    threadId = thrNo;

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
    RRLogEntry *e;

    if (rrMode == RRMODE_NORMAL) {
	return _pthread_create(thread, attr, start_routine, arg);
    }

    if (rrMode == RRMODE_RECORD) {
	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_THREAD_CREATE;
	thrNo = RRShared_AllocThread(rrlog);
	e->value[1] = thrNo;
	RRLog_Append(rrlog, e);
    } else {
	e = RRPlay_Dequeue(rrlog, threadId);
	thrNo = e->value[1];
	AssertEvent(e, RREVENT_THREAD_CREATE);
	RRPlay_Free(rrlog, e);
	RRShared_SetupThread(rrlog, thrNo);
    }

    threadState[thrNo].start = start_routine;
    threadState[thrNo].arg = arg;
    result = _pthread_create(thread, attr, thrwrapper, (void *)thrNo);
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
pthread_barrier_destroy(pthread_barrier_t *barrier)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_BARRIER_DESTROY;
	    e->objectId = (uint64_t)barrier;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, threadId);
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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_BARRIER_INIT;
	    e->objectId = (uint64_t)barrier;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, threadId);
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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_BARRIER_WAIT;
	    e->objectId = (uint64_t)barrier;
	    RRLog_Append(rrlog, e);
	    result = _pthread_barrier_wait(barrier);
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_BARRIER_WAIT;
	    e->objectId = (uint64_t)barrier;
	    e->value[0] = (uint64_t)result;
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_BARRIER_WAIT);
	    RRPlay_Free(rrlog, e);
	    e = RRPlay_Dequeue(rrlog, threadId);
	    result = (int)e->value[0];
	    AssertEvent(e, RREVENT_BARRIER_WAIT);
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
	    e->value[4] = __builtin_readcyclecounter(); //XXX: where is this used?
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
pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abs_timeout)
{
    int result = -1;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL: {
	    result = _pthread_mutex_timedlock(mutex, abs_timeout);
	    break;
	}
	case RRMODE_RECORD: {
	    RRLog_LEnter(threadId, (uint64_t)mutex);

	    result = _pthread_mutex_timedlock(mutex, abs_timeout);

	    e = RRLog_LAlloc(threadId);
	    e->event = RREVENT_MUTEX_TIMEDLOCK;
	    e->objectId = (uint64_t)mutex;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_LAppend(e);
	    break;
	}
	case RRMODE_REPLAY: {
	    RRPlay_LEnter(threadId, (uint64_t)mutex);
	    e = RRPlay_LDequeue(threadId);
	    AssertEvent(e, RREVENT_MUTEX_TIMEDLOCK);
      
      /* Record call was successful, wait until replay call matches */
      if (e->value[0] == 0) {
        while (result != 0) {
          result = _pthread_mutex_timedlock(mutex, abs_timeout);
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
pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    break;
	case RRMODE_RECORD:
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_SPIN_INIT;
	    e->objectId = (uint64_t)lock;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, threadId);
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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_SPIN_DESTROY;
	    e->objectId = (uint64_t)lock;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, threadId);
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
	    RRLog_LEnter(threadId, (uint64_t)lock);

	    result = _pthread_spin_lock(lock);

	    e = RRLog_LAlloc(threadId);
	    e->event = RREVENT_SPIN_LOCK;
	    e->objectId = (uint64_t)lock;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_LAppend(e);
	    break;
	}
	case RRMODE_REPLAY: {
	    RRPlay_LEnter(threadId, (uint64_t)lock);

	    result = _pthread_spin_lock(lock);

	    e = RRPlay_LDequeue(threadId);
	    AssertEvent(e, RREVENT_SPIN_LOCK);
	    // ASSERT result = e->value[0];
	    RRPlay_LFree(e);
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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_SPIN_TRYLOCK;
	    e->objectId = (uint64_t)lock;
	    result = _pthread_spin_trylock(lock);
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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_SPIN_UNLOCK;
	    e->objectId = (uint64_t)lock;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    result = _pthread_spin_unlock(lock);
	    e = RRPlay_Dequeue(rrlog, threadId);
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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_RWLOCK_INIT;
	    e->objectId = (uint64_t)lock;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, threadId);
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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_RWLOCK_DESTROY;
	    e->objectId = (uint64_t)lock;
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, threadId);
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
	    RRLog_LEnter(threadId, (uint64_t)lock);

	    result = _pthread_rwlock_rdlock(lock);

	    e = RRLog_LAlloc(threadId);
	    e->event = RREVENT_RWLOCK_RDLOCK;
	    e->objectId = (uint64_t)lock;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_LAppend(e);
	    break;
	}
	case RRMODE_REPLAY: {
	    RRPlay_LEnter(threadId, (uint64_t)lock);

	    result = _pthread_rwlock_rdlock(lock);

	    e = RRPlay_LDequeue(threadId);
	    AssertEvent(e, RREVENT_RWLOCK_RDLOCK);
	    // ASSERT result = e->value[0];
	    RRPlay_LFree(e);
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
	    RRLog_LEnter(threadId, (uint64_t)lock);

	    result = _pthread_rwlock_timedrdlock(lock, abs_timeout);

	    e = RRLog_LAlloc(threadId);
	    e->event = RREVENT_RWLOCK_TIMEDRDLOCK;
	    e->objectId = (uint64_t)lock;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_LAppend(e);
	    break;
	}
	case RRMODE_REPLAY: {
	    RRPlay_LEnter(threadId, (uint64_t)lock);
	    e = RRPlay_LDequeue(threadId);
	    AssertEvent(e, RREVENT_RWLOCK_TIMEDRDLOCK);

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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_RWLOCK_TRYRDLOCK;
	    e->objectId = (uint64_t)lock;
	    result = _pthread_rwlock_tryrdlock(lock);
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
	    RRLog_LEnter(threadId, (uint64_t)lock);

	    result = _pthread_rwlock_wrlock(lock);

	    e = RRLog_LAlloc(threadId);
	    e->event = RREVENT_RWLOCK_WRLOCK;
	    e->objectId = (uint64_t)lock;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_LAppend(e);
	    break;
	}
	case RRMODE_REPLAY: {
	    RRPlay_LEnter(threadId, (uint64_t)lock);

	    result = _pthread_rwlock_wrlock(lock);

	    e = RRPlay_LDequeue(threadId);
	    AssertEvent(e, RREVENT_RWLOCK_WRLOCK);
	    // ASSERT result = e->value[0];
	    RRPlay_LFree(e);
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
	    RRLog_LEnter(threadId, (uint64_t)lock);

	    result = _pthread_rwlock_timedwrlock(lock, abs_timeout);

	    e = RRLog_LAlloc(threadId);
	    e->event = RREVENT_RWLOCK_TIMEDWRLOCK;
	    e->objectId = (uint64_t)lock;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_LAppend(e);
	    break;
	}
	case RRMODE_REPLAY: {
	    RRPlay_LEnter(threadId, (uint64_t)lock);
	    e = RRPlay_LDequeue(threadId);
	    AssertEvent(e, RREVENT_RWLOCK_TIMEDWRLOCK);

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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_RWLOCK_TRYWRLOCK;
	    e->objectId = (uint64_t)lock;
	    result = _pthread_rwlock_trywrlock(lock);
	    e->value[0] = (uint64_t)result;
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    e = RRPlay_Dequeue(rrlog, threadId);
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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_RWLOCK_UNLOCK;
	    e->objectId = (uint64_t)lock;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    result = _pthread_rwlock_unlock(lock);
	    e = RRPlay_Dequeue(rrlog, threadId);
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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_THREAD_DETACH;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    result = _pthread_detach(thread);
	    e = RRPlay_Dequeue(rrlog, threadId);
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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_THREAD_JOIN;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    result = _pthread_join(thread, value_ptr);
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_THREAD_JOIN);
	    RRPlay_Free(rrlog, e);
	    break;
	}
    }

    return result;
}

int
_pthread_timedjoin_np(pthread_t thread, void **value_ptr,
         const struct timespec *abstime)
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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_THREAD_TIMEDJOIN;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter();
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_THREAD_TIMEDJOIN);

      /* Record call was successful, wait until replay call matches */
      if (e->value[0] == 0) {
        while (result != 0) {
          result = _pthread_join(thread, value_ptr);
        }
      } else {
        result = e->value[0];
      }
      
	    RRPlay_Free(rrlog, e);
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
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_THREAD_KILL;
	    e->value[0] = (uint64_t)result;
	    e->value[4] = __builtin_readcyclecounter(); //XXX: why is this here
	    RRLog_Append(rrlog, e);
	    break;
	}
	case RRMODE_REPLAY: {
	    result = _pthread_kill(thread, sig);
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_THREAD_KILL);
	    RRPlay_Free(rrlog, e);
	    break;
	}
    }

    return result;
}

__strong_reference(_pthread_mutex_lock, pthread_mutex_lock);
