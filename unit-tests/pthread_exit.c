#include <stdio.h>
#include <assert.h>

#include <pthread.h>

void * 
routine(void *arg)
{
    pthread_exit(NULL);
    return NULL;
}

int
main() {
    int status;
    pthread_t t1;

    status = pthread_create(&t1, NULL, &routine, (void *)1);
    assert(status == 0);

    pthread_join(t1, NULL);
    pthread_exit(NULL);
}
