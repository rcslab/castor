#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

int main(int argc, const char *argv[])
{
    int result;
    char *t;
    time_t tm;

    time(&tm);
    t = ctime(&tm);
    printf("%s", t);

    struct timeval tv;
    result = gettimeofday(&tv,NULL);
    assert(result == 0);
    printf("%lx,%lx\n", tv.tv_sec, tv.tv_usec);

    struct timespec ts;
    result = clock_gettime(CLOCK_REALTIME_FAST, &ts);
    assert(result == 0);
    printf("%lx,%lx\n", ts.tv_sec, ts.tv_nsec);
}

