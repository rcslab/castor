
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

int main(int argc, const char *argv[])
{
    int fd, fd2, p[2];
    int status;
    struct stat sb;

    // Test fd mmap
    fd = open("read.c", O_RDONLY);
    assert(fd != -1);
    printf("%d = open()\n", fd);

    status = fstat(fd, &sb);
    assert(status == 0);

    fd2 = dup(fd);
    assert(fd2 != -1);
    printf("%d = dup(%d)\n", fd2, fd);

    status = dup2(fd, 10);
    assert(status == 10);

    status = close(10);
    assert(status == 0);

    status = close(fd);
    assert(status == 0);

    status = pipe((int *)&p);
    assert(status == 0);
    printf("p = { %d, %d }\n", p[0], p[1]);

    return 0;
}

