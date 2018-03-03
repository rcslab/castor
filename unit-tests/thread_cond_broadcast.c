#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <pthread.h>

pthread_mutex_t mtx;
pthread_cond_t cond;
int counter = 0;

void *
threadA(void *arg)
{
    printf("starting thread A");
    fflush(stdout);
    pthread_mutex_lock(&mtx);
    while (counter != 1)
    {
	printf("ThreadA: All I can do is block *sigh*!\n");
	pthread_cond_wait(&cond, &mtx);
    }
    printf("ThreadA: Counter was incremented, I'm free!\n");
    pthread_mutex_unlock(&mtx);

    return NULL;
}

void *
threadB(void *arg)
{

    printf("starting thread B");
    fflush(stdout);
    pthread_mutex_lock(&mtx);
    while (counter != 1)
    {
	printf("ThreadB: All I can do is block *sigh*!\n");
	pthread_cond_wait(&cond, &mtx);
    }
    printf("ThreadB: Counter was incremented, I'm free!\n");
    pthread_mutex_unlock(&mtx);

    return NULL;
}

void *
threadC(void *arg)
{
    printf("starting thread C");
    fflush(stdout);
    pthread_mutex_lock(&mtx);
    printf("ThreadC: These threads are so needy, I have to do everything\n");
    counter = 1;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mtx);
    return NULL;
}

int main(int argc, const char *argv[])
{
    int status;
    pthread_t t1;
    pthread_t t2;
    pthread_t t3;

    status = pthread_mutex_init(&mtx, NULL);
    assert(status == 0);

    status = pthread_cond_init(&cond, NULL);
    assert(status == 0);

    status = pthread_create(&t1, NULL, &threadA, (void *)2);
    assert(status == 0);
    status = pthread_create(&t2, NULL, &threadB, (void *)3);
    assert(status == 0);
    status = pthread_create(&t3, NULL, &threadC, (void *)4);
    assert(status == 0);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    printf("All done!\n");
    return 0;
}

