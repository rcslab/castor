
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

#include <libc_private.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "util.h"

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

    return -1;
}

extern int __sys_shmget(key_t key, size_t size, int flag);
extern void *__sys_shmat(int shmid, const void *addr, int flag);

/*
 * XXX: Several sources of nondeterminism left in shmget/shmat/shmdt/shmctl 
 * when threads/procs race to make calls at the same time.
 */

int
shmget(key_t key, size_t size, int flag)
{
    int result = 0;
    RRLogEntry *e;


    switch (rrMode) {
	case RRMODE_NORMAL:
	    return __sys_shmget(key, size, flag);
	case RRMODE_RECORD:
	    result = __sys_shmget(key, size, flag);
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
		result = __sys_shmget(key, size, flag);
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
shmat(int shmid, const void *addr, int flag)
{
    switch (rrMode) {
	case RRMODE_NORMAL:
	case RRMODE_RECORD:
	    return __sys_shmat(shmid, addr, flag);
	case RRMODE_REPLAY: {
	    int rShmid = sysv_lookup(shmid);
	    return __sys_shmat(rShmid, addr, flag);
	}
    }
}

