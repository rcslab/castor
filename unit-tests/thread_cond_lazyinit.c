#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mtx;

void *
routine(void *arg)
{
    pthread_mutex_lock(&mtx);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mtx);
    return NULL;
}

int
main()
{
    int status;
    pthread_t t1;

    pthread_mutex_init(&mtx, NULL);
    pthread_mutex_lock(&mtx);

    status = pthread_create(&t1, NULL, &routine, (void *)1);
    assert(status == 0);

    pthread_cond_wait(&cond, &mtx);

    pthread_mutex_unlock(&mtx);

    pthread_join(t1, NULL);
}
