#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <pthread.h>

#define ITERATIONS 5
#define SLEEP_TIME_NS 10000000 // 10ms
#define MUTEX_TOUT_NS SLEEP_TIME_NS/2 

pthread_mutex_t mtx;

void *
routine(void *arg)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += MUTEX_TOUT_NS;

    for (int i = 0; i < ITERATIONS; i++)
    {
	if (pthread_mutex_timedlock(&mtx, &ts) == 0) {
	    usleep(SLEEP_TIME_NS / 1000);
	    pthread_mutex_unlock(&mtx);
	}
    }

    return NULL;
}

int main(int argc, const char *argv[])
{
    int status;
    pthread_t t1;
    pthread_t t2;

    status = pthread_mutex_init(&mtx, NULL);
    assert(status == 0);

    status = pthread_create(&t1, NULL, &routine, (void *)1);
    assert(status == 0);
    status = pthread_create(&t2, NULL, &routine, (void *)2);
    assert(status == 0);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
}

