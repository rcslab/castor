
#include <assert.h>
#include <stdio.h>

#include <pthread.h>

void *
routine(void *arg)
{
    int thrNo = (int)arg;

    printf("Hello %d\n", thrNo);

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

