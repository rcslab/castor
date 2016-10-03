#include <sys/param.h>
#include <sys/mount.h>

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "test_helpers.h"

int main(int argc, const char *argv[])
{
    int fd;
    struct statfs buf;
    fd = open(".", O_RDONLY);
    fstatfs(fd, &buf);
    hex_dump((char *)&buf, sizeof(buf));
    return 0;
}
