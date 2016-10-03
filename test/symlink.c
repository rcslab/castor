#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
    int result;
    char buf[255];
    unlink("link.c.link");
    result = symlink("link.c", "link.c.link");
    assert(result == 0);
    result = readlink("link.c.link", buf, 255);
    assert(result > 0);
    assert(strcmp("link.c", buf) == 0);
    result = unlink("link.c.link");
    assert(result == 0);

    return 0;
}




