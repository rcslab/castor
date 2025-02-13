
#include <assert.h>
#include <stdio.h>

#include <pthread.h>

#define ITERATIONS 5000

pthread_mutex_t mtx;
int value;

void *
routine(void *arg)
{
    for (int i = 0; i < ITERATIONS; i++)
    {
	if (pthread_mutex_trylock(&mtx) == 0) {
		value++;
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

    value = 0;

    status = pthread_mutex_init(&mtx, NULL);
    assert(status == 0);

    status = pthread_create(&t1, NULL, &routine, (void *)1);
    assert(status == 0);
    status = pthread_create(&t2, NULL, &routine, (void *)2);
    assert(status == 0);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Value is %d.\n", value);
}

