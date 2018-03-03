#include <assert.h>
#include <stdio.h>

#include <unistd.h>

int main(int argc, const char *argv[])
{
    uid_t result;
    int i;
    printf("start test\n");
    fflush(NULL);
    result = getuid();
    printf("result %d\n", result);
    result = geteuid();
    printf("result %d\n", result);
    result = getgid();
    printf("result %d\n", result);
    result = getegid();
    printf("result %d\n", result);
    i = setuid(getuid());
    assert(i == 0);
    i = seteuid(geteuid());
    assert(i == 0);
    i = setgid(getgid());
    assert(i == 0);
    i = setegid(getgid());
    assert(i == 0);
}
