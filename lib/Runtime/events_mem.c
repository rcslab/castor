
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

// mmap
#include <sys/mman.h>

#include <libc_private.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "fdinfo.h"
#include "util.h"

extern void *__sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);

void *
__rr_mmap_log(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    void *result;
    RRLogEntry *e;

    if (rrMode == RRMODE_RECORD) {
	result = __sys_mmap(addr, len, prot, flags, fd, offset);
	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_MMAPFD;
	e->value[0] = (uint64_t)result;
	if (result == MAP_FAILED) {
	    e->value[1] = (uint64_t)errno;
	}
	e->value[2] = len;
	e->value[3] = (uint64_t)prot;
	e->value[4] = (uint64_t)flags;
	RRLog_Append(rrlog, e);

	if (result != MAP_FAILED) {
	    logData((uint8_t *)result, len);
	}
    } else {
	e = RRPlay_Dequeue(rrlog, threadId);
	AssertEvent(e, RREVENT_MMAPFD);
	result = (void *)e->value[0];
	if (result == MAP_FAILED) {
	    errno = (int)e->value[1];
	}
	RRPlay_Free(rrlog, e);

	if (result != MAP_FAILED) {
	    void *origAddr = result;
	    result = __sys_mmap(origAddr, len, prot | PROT_WRITE, flags | MAP_ANON, -1, 0);
	    ASSERT_IMPLEMENTED(result != MAP_FAILED);
	    if (origAddr != result) {
		WARNING("mmap replay didn't use the same address (%p -> %p)", origAddr, result);
	    }
	    logData(result, len);
	    if ((prot & PROT_WRITE) == 0) {
		int status = syscall(SYS_mprotect, addr, len, prot);
		ASSERT_IMPLEMENTED(status == 0);
	    }
	}
    }

    return result;
}

void *
__rr_mmap_shared(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    void *result;
    RRLogEntry *e;

    if (rrMode == RRMODE_RECORD) {
	result = __sys_mmap(addr, len, prot, flags, fd, offset);
	e = RRLog_Alloc(rrlog, threadId);
	e->event = RREVENT_MMAPFD;
	e->value[0] = (uint64_t)result;
	if (result == MAP_FAILED) {
	    e->value[1] = (uint64_t)errno;
	}
	e->value[2] = len;
	e->value[3] = (uint64_t)prot;
	e->value[4] = (uint64_t)flags;
	RRLog_Append(rrlog, e);
    } else {
	switch (FDInfo_GetType(fd)) {
	    case FDTYPE_SHM:
		break;
	    default:
		NOT_IMPLEMENTED();
	}
	int realFd = FDInfo_GetFD(fd);

	e = RRPlay_Dequeue(rrlog, threadId);
	AssertEvent(e, RREVENT_MMAPFD);
	result = (void *)e->value[0];
	if (result == MAP_FAILED) {
	    errno = (int)e->value[1];
	}
	RRPlay_Free(rrlog, e);

	if (result != MAP_FAILED) {
	    void *origAddr = result;
	    result = __sys_mmap(origAddr, len, prot, flags, realFd, offset);
	    ASSERT_IMPLEMENTED(result != MAP_FAILED);
	    if (origAddr != result) {
		WARNING("mmap replay didn't use the same address (%p -> %p)", origAddr, result);
	    }
	}
    }

    return result;
}

void *
__rr_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    if (rrMode == RRMODE_NORMAL) {
	return __sys_mmap(addr, len, prot, flags, fd, offset);
    }

    /*
     * Both anonymous shared and private mmap'ed regions should work without 
     * modification.
     */
    if (fd == -1) {
	return __sys_mmap(addr, len, prot, flags, fd, offset);
    }

    if (flags & MAP_PRIVATE) {
	return __rr_mmap_log(addr, len, prot, flags, fd, offset);
    } else if (flags & MAP_SHARED) {
	return __rr_mmap_shared(addr, len, prot, flags, fd, offset);
    } else {
	NOT_IMPLEMENTED();
    }
}

BIND_REF(mmap);

