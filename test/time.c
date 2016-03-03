
#include <stdio.h>
#include <time.h>

int main(int argc, const char *argv[])
{
    time_t tm;
    char *t;

    time(&tm);
    t = ctime(&tm);

    printf("%s", t);
}

