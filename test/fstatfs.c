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
    int result;
    struct statfs buf;
    fd = open(".", O_RDONLY);
    fstatfs(fd, &buf);
    result = faccessat(fd, "fstatfs.c", R_OK, 0);
    assert(result == 0);
    hex_dump((char *)&buf, sizeof(buf));
    statfs(".", &buf);
    hex_dump((char *)&buf, sizeof(buf));
    return 0;
}
