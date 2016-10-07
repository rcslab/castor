#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
    int result;
    char buf[255];
    unlink("symlink.c.link");
    result = symlink("symlink.c", "symlink.c.link");
    assert(result == 0);
    result = readlink("symlink.c.link", buf, 255);
    assert(result > 0);
    printf("[%s]",buf);
    assert(memcmp("symlink.c", buf, strlen("symlink.c")) == 0);
    result = unlink("symlink.c.link");
    assert(result == 0);

    return 0;
}




