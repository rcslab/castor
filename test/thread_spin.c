
#include <assert.h>
#include <stdio.h>

#include <pthread.h>

#define ITERATIONS 5000

pthread_spinlock_t lock;

void *
routine(void *arg)
{
    for (int i = 0; i < ITERATIONS; i++)
    {
	pthread_spin_lock(&lock);
	pthread_spin_unlock(&lock);
    }

    return NULL;
}

int main(int argc, const char *argv[])
{
    int status;
    pthread_t t1;
    pthread_t t2;

    status = pthread_spin_init(&lock, 0);
    assert(status == 0);

    status = pthread_create(&t1, NULL, &routine, (void *)1);
    assert(status == 0);
    status = pthread_create(&t2, NULL, &routine, (void *)2);
    assert(status == 0);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
}

