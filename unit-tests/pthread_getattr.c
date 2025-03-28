
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <pthread_np.h>

void *
routine(void *arg)
{
    // thread_get_attr triggers jemalloc internal tsd allocation
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_get_np(pthread_self(), &attr);
    pthread_attr_destroy(&attr);
    
    return NULL;
}

int main(int argc, const char *argv[])
{
    int status;
    pthread_t t1;
    pthread_t t2;

    status = pthread_create(&t1, NULL, &routine, (void *)1);
    assert(status == 0);
    status = pthread_create(&t2, NULL, &routine, (void *)2);
    assert(status == 0);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
}

