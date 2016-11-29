#include <stdio.h>
#include <unistd.h>

int main()
{
    char buf1[1024];

    getcwd(buf1, sizeof(buf1));
    printf("%s\n", buf1);
}
