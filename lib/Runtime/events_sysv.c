
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

#include <stdatomic.h>
#include <threads.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#define _WANT_SEMUN
#include <sys/sem.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/cdefs.h>
#include <sys/syscall.h>

#include <libc_private.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "util.h"
#include "system.h"

#define SYSV_MAX_ENTRIES 64

typedef struct SysVEntry {
    atomic_int key;
    atomic_int val;
} SysVEntry;

typedef struct SysVMap {
    SysVEntry entries[SYSV_MAX_ENTRIES];
} SysVMap;

static SysVMap *sysvdesc;

static void
sysv_alloc()
{
    if (rrlog->sysvmap == 0) {
	// XXX: Leaking shared region
	uintptr_t off = RRShared_AllocRegion(rrlog, sizeof(SysVMap));
	uintptr_t e = 0;

	SysVMap *vm = (SysVMap *)((uintptr_t)rrlog + off);
	for (int i = 0; i < SYSV_MAX_ENTRIES; i++) {
	    vm->entries[i].key = 0;
	    vm->entries[i].val = 0;
	}

	atomic_compare_exchange_strong(&rrlog->sysvmap, &e, off);
    }

    sysvdesc = (SysVMap *)((uintptr_t)rrlog + atomic_load(&rrlog->sysvmap));
}

static void
sysv_insert(int key, int val)
{
    sysv_alloc();

    for (int i = 0; i < SYSV_MAX_ENTRIES; i++) {
	int exp = 0;
	if (atomic_compare_exchange_strong(&sysvdesc->entries[i].val, &exp, val)) {
	    atomic_store(&sysvdesc->entries[i].key, key);
	    return;
	}
    }
}

static int
sysv_lookup(int key)
{
    sysv_alloc();

    for (int i = 0; i < SYSV_MAX_ENTRIES; i++) {
      if (atomic_load(&sysvdesc->entries[i].key) == key) {
	    return atomic_load(&sysvdesc->entries[i].val);
	}
    }

    DLOG("sysv_lookup(%d) was unable to find key.", key);
    errno = EINVAL;
    return -1;
}

/*
 * XXX: Several sources of nondeterminism left in shmget/shmat/shmdt/shmctl 
 * when threads/procs race to make calls at the same time.
 */

int
__rr_shmget(key_t key, size_t size, int flag)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return __rr_syscall(SYS_shmget, key, size, flag);
	case RRMODE_RECORD:
	    result = __rr_syscall(SYS_shmget, key, size, flag);
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_SHMGET;
	    e->objectId = (uint64_t)result;
	    if (result == -1) {
		e->value[1] = (uint64_t)errno;
	    }
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_SHMGET);
	    if ((int)e->objectId == -1) {
		errno = e->value[1];
	    } else {
		result = __rr_syscall(SYS_shmget, key, size, flag);
		sysv_insert(e->objectId, result);
	    }
	    result = e->objectId;
	    RRPlay_Free(rrlog, e);
	    break;
    }

    return result;
}

/*
 * XXX: Need to prevent randomization of the address on replay
 */
void *
__rr_shmat(int shmid, const void *addr, int flag)
{
    switch (rrMode) {
	case RRMODE_NORMAL:
	case RRMODE_RECORD:
	    return (void *)__rr_syscall_long(SYS_shmat, shmid, addr, flag);
	case RRMODE_REPLAY: {
	    int rShmid = sysv_lookup(shmid);
	    return (void *)__rr_syscall_long(SYS_shmat, rShmid, addr, flag);
	}
    }
}

int
__rr_semget(key_t key, int nsems, int flag)
{
    int result = 0;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return __rr_syscall(SYS_semget, key, nsems, flag);
	case RRMODE_RECORD:
	    result = __rr_syscall(SYS_semget, key, nsems, flag);
	    e = RRLog_Alloc(rrlog, threadId);
	    e->event = RREVENT_SEMGET;
	    e->objectId = (uint64_t)result;
	    if (result == -1) {
		e->value[1] = (uint64_t)errno;
	    }
	    RRLog_Append(rrlog, e);
	    break;
	case RRMODE_REPLAY:
	    e = RRPlay_Dequeue(rrlog, threadId);
	    AssertEvent(e, RREVENT_SEMGET);
	    if ((int)e->objectId == -1) {
		errno = e->value[1];
	    } else {
		result = __rr_syscall(SYS_semget, key, nsems, flag);
		sysv_insert(e->objectId, result);
	    }
	    result = e->objectId;
	    RRPlay_Free(rrlog, e);
	    break;
    }

    return result;
}

int
__rr_semop(int semid, struct sembuf *array, size_t nops)
{
    switch (rrMode) {
	case RRMODE_NORMAL:
	case RRMODE_RECORD:
	    return __rr_syscall(SYS_semop, semid, array, nops);
	case RRMODE_REPLAY: {
	    int rSemid = sysv_lookup(semid);
	    return __rr_syscall(SYS_semop, rSemid, array, nops);
	}
    }
}

int
__rr___semctl(int semid, int semnum, int cmd, union semun *arg)
{
    int result;
    RRLogEntry *e;

    switch (rrMode) {
	case RRMODE_NORMAL:
	    return __rr_syscall(SYS___semctl, semid, semnum, cmd, arg);
	case RRMODE_RECORD:
	    Mutex_Lock(&rrlog->sysvlck);
	    e = RRLog_Alloc(rrlog, threadId);
	    result = __rr_syscall(SYS___semctl, semid, semnum, cmd, arg);
	    e->event = RREVENT_SEMCTL;
	    e->objectId = (uint64_t)semid;
	    if (cmd == GETPID) {
		e->value[0] = (uint64_t)result;
	    }
	    RRLog_Append(rrlog, e);
	    Mutex_Unlock(&rrlog->sysvlck);
	    break;
	case RRMODE_REPLAY: {
	    Mutex_Lock(&rrlog->sysvlck);
	    e = RRPlay_Dequeue(rrlog, threadId);
	    int rSemid = sysv_lookup(semid);
	    if (cmd == GETPID) {
		result = (int) e->value[0];
	    } else {
		result = __rr_syscall(SYS___semctl, rSemid, semnum, cmd, arg);
	    }
	    AssertEvent(e, RREVENT_SEMCTL);
	    RRPlay_Free(rrlog, e);
	    Mutex_Unlock(&rrlog->sysvlck);
	    break;
	}
    }

    return result;
}

int
__rr_semctl(int semid, int semnum, int cmd, ...)
{
	va_list ap;
	union semun s;
	union semun *ptr;

	va_start(ap, cmd);
	if ((cmd == IPC_SET) || (cmd == IPC_STAT) || (cmd == GETALL) ||
	    (cmd == SETVAL) || (cmd == SETALL)) {
		s = va_arg(ap, union semun);
		ptr = &s;
	} else {
		ptr = NULL;
	}
	va_end(ap);

	return __rr_syscall(SYS___semctl, semid, semnum, cmd, ptr);
}

BIND_REF(semctl);
BIND_REF(semop);
BIND_REF(shmat);
BIND_REF(shmget);
BIND_REF(semget);
