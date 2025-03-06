
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>
#include <pthread_np.h>

#define ITERATIONS 5000

void *
routine(void *arg)
{
    intptr_t dur = (intptr_t)arg;
    printf("Sleep %ld microseconds..\n", dur);
    usleep(dur);
    printf("Up\n");
    return NULL;
}

int main(int argc, const char *argv[])
{
    int status;
    struct timespec tmout;
    pthread_t t1, t2, t3;

    status = pthread_create(&t1, NULL, &routine, (void *)50000);
    assert(status == 0);
    status = pthread_create(&t2, NULL, &routine, (void *)1000);
    assert(status == 0);
    status = pthread_create(&t3, NULL, &routine, (void *)0);
    assert(status == 0);

    if (clock_gettime(CLOCK_REALTIME, &tmout) == -1) {
	assert(0);
    }
    tmout.tv_sec += 0;
    tmout.tv_nsec += 20000000;

    status = pthread_timedjoin_np(t1, NULL, &tmout);
    assert(status == ETIMEDOUT);

    status = pthread_timedjoin_np(t2, NULL, &tmout);
    assert(status == 0);

    status = pthread_timedjoin_np(t3, NULL, NULL);
    assert(status == EINVAL);
}

