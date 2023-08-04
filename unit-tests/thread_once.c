
#include <assert.h>
#include <stdio.h>
#include <threads.h>

#include <unistd.h>
#include <pthread.h>

pthread_once_t oc1;
pthread_once_t oc2;
thread_local intptr_t thrNo;

void
initroutine(void)
{
    printf("init %ld\n", thrNo);
}

void *
routine(void *arg)
{
    thrNo = (intptr_t)arg;

    pthread_once(&oc2, initroutine);
    pthread_once(&oc2, initroutine);

    return NULL;
}

int main(int argc, const char *argv[])
{
    int status;
    pthread_t t1;
    pthread_t t2;

    thrNo = 0;

    pthread_once(&oc1, initroutine);
    pthread_once(&oc1, initroutine);

    status = pthread_create(&t1, NULL, &routine, (void *)1);
    assert(status == 0);
    status = pthread_create(&t2, NULL, &routine, (void *)2);
    assert(status == 0);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
}

