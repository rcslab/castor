
#include <unistd.h>
#include <stdio.h>

int main(int argc, const char *argv[])
{
    int result;
    printf("start test\n");
    fflush(NULL);
    result = getuid();
    printf("result %d\n",result);
}
