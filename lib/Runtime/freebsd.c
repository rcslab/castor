
#include <unistd.h>
#include <sys/syscall.h>

#include "system.h"

int
SystemRead(int fd, void *buf, size_t nbytes)
{
    return syscall(SYS_read, fd, buf, nbytes);
}

int
SystemWrite(int fd, const void *buf, size_t nbytes)
{
    return syscall(SYS_write, fd, buf, nbytes);
}

int
SystemGetpid()
{
    return syscall(SYS_getpid);
}

