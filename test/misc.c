#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <sys/capsicum.h>

int main()
{
    int result;
    struct rlimit rlin = { .rlim_max = 500 };
    struct rlimit rlout;
    result = setrlimit(RLIMIT_CORE, &rlin);
    assert(result == 0);
    result = getrlimit(RLIMIT_CORE, &rlout);
    assert(result == 0);
    assert(rlin.rlim_max == rlout.rlim_max);

    struct rusage ru1, ru2;
    result = getrusage(0, &ru1);
    assert(result == 0);
    result = getrusage(0, &ru2);
    assert(result == 0);

    printf("%ld\n", ru1.ru_stime.tv_usec);
    printf("%ld\n", ru2.ru_stime.tv_usec);
    assert(ru2.ru_stime.tv_usec > ru1.ru_stime.tv_usec);
    result = cap_enter();
    assert(result == 0);
}
