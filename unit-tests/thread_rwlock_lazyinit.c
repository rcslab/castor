#include <stdio.h>
#include <assert.h>

#include <pthread.h>

pthread_rwlock_t rw = PTHREAD_RWLOCK_INITIALIZER;

void *
routine(void *arg)
{
    pthread_rwlock_rdlock(&rw);
    pthread_rwlock_unlock(&rw);
    return NULL;
}


int
main()
{
    int status;
    pthread_t t1;

    status = pthread_create(&t1, NULL, &routine, (void *)1);
    assert(status == 0);
    pthread_join(t1, NULL);
}
