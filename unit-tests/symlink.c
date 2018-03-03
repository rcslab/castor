#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int main()
{
    int result;
    char buf[255];
    unlink("symlink.c.link");
    result = symlink("symlink.c", "symlink.c.link");
    assert(result == 0);
    result = readlink("symlink.c.link", buf, 255);
    assert(result > 0);
    printf("[%s]\n",buf);
    result = readlinkat(AT_FDCWD, "symlink.c.link", buf, 255);
    assert(result > 0);
    printf("[%s]\n",buf);

    assert(memcmp("symlink.c", buf, strlen("symlink.c")) == 0);
    result = unlink("symlink.c.link");
    assert(result == 0);

    unlink("test.txt");
    int fd = open("test.txt", O_CREAT | O_TRUNC | O_RDWR);
    assert(fd > 0);
    result = fchmod(fd, S_IRWXU);
    assert(result == 0);
    int cwd = open(".", O_RDONLY);
    assert(cwd > 0);
    result = linkat(cwd, "test.txt", cwd, "test2.txt", 0);
    assert(result == 0);
    result = unlinkat(AT_FDCWD, "test.txt", 0);
    assert(result == 0);
    result = lchmod("test2.txt", S_IXUSR);
    assert(result == 0);
    result = fchmodat(AT_FDCWD, "test2.txt", S_IRWXU, 0);
    assert(result == 0);
    result = unlinkat(cwd, "test2.txt", 0);
    assert(result == 0);

    return 0;
}

