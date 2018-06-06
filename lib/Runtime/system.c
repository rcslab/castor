
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/cdefs.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "system.h"
#include "util.h"

int
SystemRead(int fd, void *buf, size_t nbytes)
{
    return __rr_syscall(SYS_read, fd, buf, nbytes);
}

int
SystemWrite(int fd, const void *buf, size_t nbytes)
{
    return __rr_syscall(SYS_write, fd, buf, nbytes);
}

int
SystemGetpid()
{
    return __rr_syscall(SYS_getpid);
}

void *
__tls_get_addr(void *ti __unused)
{

	return (0);
}

