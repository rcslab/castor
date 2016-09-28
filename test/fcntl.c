
#include <assert.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>

#include <castor/rr_debug.h>

int main(int argc, const char *argv[])
{
    int fd;
    int flags = O_NONBLOCK | O_APPEND;
    int result;
    fd = open("fcntl.c", O_RDONLY);
    result = fcntl(fd, F_SETFL, flags);
    assert(result == 0);
    result = fcntl(fd, F_GETFL, flags);
    assert(result == flags);
    return 0;
}

