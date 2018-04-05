
#include <stdbool.h>
#include <stdarg.h>
#include <threads.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "util.h"
#include "fdinfo.h"

extern int __sys_shm_open(const char *path, int flags, mode_t mode);
extern int __sys_shm_unlink(const char *path);

// XXX: Patch libc to switch to new sem_* calls
extern int _libc_sem_init_compat(sem_t *, int, unsigned int);
extern int _libc_sem_destroy_compat(sem_t *);
extern sem_t *_libc_sem_open_compat(const char *, int, ...);
extern int _libc_sem_close_compat(sem_t *);
extern int _libc_sem_unlink_compat(const char *);
extern int _libc_sem_post_compat(sem_t *);
extern int _libc_sem_wait_compat(sem_t *);
extern int _libc_sem_trywait_compat(sem_t *);
extern int _libc_sem_timedwait_compat(sem_t * __restrict, const struct timespec * __restrict);
extern int _libc_sem_getvalue_compat(sem_t * __restrict, int * __restrict);

int
__rr_shm_open(const char *path, int flags, mode_t mode)
{
    int status;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return __sys_shm_open(path, flags, mode);
	case RRMODE_RECORD: {
	    status = __sys_shm_open(path, flags, mode);
	    RRRecordI(RREVENT_SHM_OPEN, status);
	    break;
	}
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SHM_OPEN, &status);
	    if (status != -1) {
		int fd = __sys_shm_open(path, flags, mode);
		ASSERT_IMPLEMENTED(fd == status);
		FDInfo_OpenShm(status, fd);
	    }
	    break;
    }

    return status;
}

int
__rr_shm_unlink(const char *path)
{
    int status;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return __sys_shm_unlink(path);
	case RRMODE_RECORD: {
	    status = __sys_shm_unlink(path);
	    RRRecordI(RREVENT_SHM_UNLINK, status);
	    break;
	}
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SHM_UNLINK, &status);
	    if (status != -1) {
		__sys_shm_unlink(path);
		// XXX: Warn of divergence if unlink fails
	    }
	    break;
    }

    return status;
}

int
__rr_sem_init(sem_t *sem, int pshared, unsigned int value)
{
    int status;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return _libc_sem_init_compat(sem, pshared, value);
	case RRMODE_RECORD: {
	    status = _libc_sem_init_compat(sem, pshared, value);
	    RRRecordI(RREVENT_SEM_INIT, status);
	    break;
	}
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SEM_INIT, &status);
	    break;
    }

    return status;
}

int
__rr_sem_destroy(sem_t *sem)
{
    int status;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return _libc_sem_destroy_compat(sem);
	case RRMODE_RECORD: {
	    status = _libc_sem_destroy_compat(sem);
	    RRRecordI(RREVENT_SEM_DESTROY, status);
	    break;
	}
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SEM_DESTROY, &status);
	    break;
    }

    return status;
}

sem_t *
__rr_sem_open(const char *name, int oflag, ...)
{
    sem_t *rval;
    mode_t mode;
    va_list ap;

    va_start(ap, oflag);
    mode = va_arg(ap, int);
    va_end(ap);

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return _libc_sem_open_compat(name, oflag, mode);
	case RRMODE_RECORD: {
	    rval = _libc_sem_open_compat(name, oflag, mode);
	    RRRecordOU(RREVENT_SEM_OPEN, 0, (uintptr_t)rval);
	    break;
	}
	case RRMODE_REPLAY: {
	    RRReplayOU(RREVENT_SEM_OPEN, 0, (uintptr_t *)&rval);
	    break;
	}
    }

    return rval;
}

int
__rr_sem_close(sem_t *sem)
{
    int status;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return _libc_sem_close_compat(sem);
	case RRMODE_RECORD: {
	    status = _libc_sem_close_compat(sem);
	    RRRecordI(RREVENT_SEM_CLOSE, status);
	    break;
	}
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SEM_CLOSE, &status);
	    break;
    }

    return status;
}

int
__rr_sem_unlink(const char *name)
{
    int status;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return _libc_sem_unlink_compat(name);
	case RRMODE_RECORD: {
	    status = _libc_sem_unlink_compat(name);
	    RRRecordI(RREVENT_SEM_UNLINK, status);
	    break;
	}
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SEM_UNLINK, &status);
	    break;
    }

    return status;
}

int
__rr_sem_post(sem_t *sem)
{
    int status;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return _libc_sem_post_compat(sem);
	case RRMODE_RECORD: {
	    RRRecordI(RREVENT_SEM_POST_START, 0);
	    status = _libc_sem_post_compat(sem);
	    RRRecordI(RREVENT_SEM_POST_END, status);
	    break;
	}
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SEM_POST_START, NULL);
	    RRReplayI(RREVENT_SEM_POST_END, &status);
	    break;
    }

    return status;
}

int
__rr_sem_wait(sem_t *sem)
{
    int status;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return _libc_sem_wait_compat(sem);
	case RRMODE_RECORD: {
	    status = _libc_sem_wait_compat(sem);
	    RRRecordI(RREVENT_SEM_WAIT, status);
	    break;
	}
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SEM_WAIT, &status);
	    break;
    }

    return status;
}

int
__rr_sem_trywait(sem_t *sem)
{
    int status;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return _libc_sem_trywait_compat(sem);
	case RRMODE_RECORD: {
	    status = _libc_sem_trywait_compat(sem);
	    RRRecordI(RREVENT_SEM_TRYWAIT, status);
	    break;
	}
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SEM_TRYWAIT, &status);
	    break;
    }

    return status;
}

int
__rr_sem_timedwait(sem_t *sem, const struct timespec *abs_timeout)
{
    int status;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return _libc_sem_timedwait_compat(sem, abs_timeout);
	case RRMODE_RECORD: {
	    status = _libc_sem_timedwait_compat(sem, abs_timeout);
	    RRRecordI(RREVENT_SEM_TIMEDWAIT, status);
	    break;
	}
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SEM_TIMEDWAIT, &status);
	    break;
    }

    return status;
}

int
__rr_sem_getvalue(sem_t *sem, int *sval)
{
    int status;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return _libc_sem_getvalue_compat(sem, sval);
	case RRMODE_RECORD: {
	    status = _libc_sem_getvalue_compat(sem, sval);
	    RRRecordI(RREVENT_SEM_GETVALUE, status);
	    logData((uint8_t *)sval, sizeof(*sval));
	    break;
	}
	case RRMODE_REPLAY:
	    RRReplayI(RREVENT_SEM_GETVALUE, &status);
	    logData((uint8_t *)sval, sizeof(*sval));
	    break;
    }

    return status;
}

BIND_REF(shm_open);
BIND_REF(shm_unlink);
BIND_REF(sem_init);
BIND_REF(sem_destroy);
BIND_REF(sem_open);
BIND_REF(sem_close);
BIND_REF(sem_unlink);
BIND_REF(sem_post);
BIND_REF(sem_wait);
BIND_REF(sem_trywait);
BIND_REF(sem_timedwait);
BIND_REF(sem_getvalue);
