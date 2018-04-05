
#include <stdint.h>

#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/syscall.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "fdinfo.h"
#include "system.h"
#include "util.h"

FDInfo fdTable[FDTABLE_SIZE];

FDType
FDInfo_GetType(int fd)
{
    ASSERT(fd < FDTABLE_SIZE);
    return fdTable[fd].type;
}

int
FDInfo_GetFD(int fd)
{
    ASSERT(fd < FDTABLE_SIZE);
    return fdTable[fd].realFd;
}

void
FDInfo_OpenFile(int fd, const char *path, int flags, int mode)
{
    ASSERT(fd < FDTABLE_SIZE);
    fdTable[fd].realFd = -1;
    fdTable[fd].type = FDTYPE_FILE;
    fdTable[fd].offset = 0;
    fdTable[fd].flags = flags;
    fdTable[fd].mode = mode;
    strncpy(&fdTable[fd].path[0], path, PATH_MAX);
}

void
FDInfo_OpenMmapLazy(int fd)
{
    ASSERT(fd < FDTABLE_SIZE);
    int f = open(fdTable[fd].path, fdTable[fd].flags, fdTable[fd].mode);
    if (f < 0) {
	PERROR("open");
    }

    fdTable[fd].realFd = f;
}

void
FDInfo_OpenShm(int fd, int realFd)
{
    ASSERT(fd < FDTABLE_SIZE);
    fdTable[fd].realFd = realFd;
    fdTable[fd].type = FDTYPE_SHM;
}

void
FDInfo_Close(int fd)
{
    ASSERT(fd < FDTABLE_SIZE);
    fdTable[fd].realFd = -1;
    fdTable[fd].type = FDTYPE_INVALID;
}

void
FDInfo_FTruncate(int fd, off_t length)
{
    int status;

    switch (FDInfo_GetType(fd)) {
	case FDTYPE_SHM:
	    status = __rr_syscall(SYS_ftruncate, FDInfo_GetFD(fd), length);
	    ASSERT_IMPLEMENTED(status != -1);
	    break;
	default:
	    break;
    }

    return;
}

