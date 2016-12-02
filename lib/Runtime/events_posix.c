
#include <stdbool.h>
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

sem_t *
__rr_sem_open(const char *name, int oflag, ...)
{
    errno = ENOSYS;
    return NULL;
}

int
__rr_sem_close(sem_t *sem)
{
    errno = ENOSYS;
    return -1;
}

int
__rr_sem_unlink(const char *name)
{
    errno = ENOSYS;
    return -1;
}

int
__rr_sem_post(sem_t *sem)
{
    errno = ENOSYS;
    return -1;
}

int
__rr_sem_wait(sem_t *sem)
{
    errno = ENOSYS;
    return -1;
}

int
__rr_sem_trywait(sem_t *sem)
{
    errno = ENOSYS;
    return -1;
}

int
__rr_sem_timedwait(sem_t *sem, const struct timespec *abs_timeout)
{
    errno = ENOSYS;
    return -1;
}

int
__rr_sem_getvalue(sem_t *sem, int *sval)
{
    errno = ENOSYS;
    return -1;
}

BIND_REF(shm_open);
BIND_REF(shm_unlink);
__strong_reference(__rr_sem_open, sem_open);
__strong_reference(__rr_sem_close, sem_close);
__strong_reference(__rr_sem_unlink, sem_unlink);
__strong_reference(__rr_sem_post, sem_post);
__strong_reference(__rr_sem_wait, sem_wait);
__strong_reference(__rr_sem_trywait, sem_trywait);
__strong_reference(__rr_sem_timedwait, sem_timedwait);
__strong_reference(__rr_sem_getvalue, sem_getvalue);

