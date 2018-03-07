#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <pthread.h>

pthread_barrier_t barrier;
int countA = 0;
int countB = 0;
int countC = 0;

void *
threadA(void *arg)
{
    printf("thread A waiting at barrier\n");
    fflush(stdout);
    countA++;
    pthread_barrier_wait(&barrier);
    assert(countB == 1);
    printf("thread A past the barrier\n");
    return NULL;
}

void *
threadB(void *arg)
{
    printf("thread B waiting at barrier\n");
    fflush(stdout);
    countB++;
    pthread_barrier_wait(&barrier);
    assert(countC == 1);
    printf("thread B past the barrier\n");
    return NULL;
}

void *
threadC(void *arg)
{
    printf("thread C waiting at barrier\n");
    fflush(stdout);
    countC++;
    pthread_barrier_wait(&barrier);
    assert(countA == 1);
    printf("thread C past the barrier\n");
    return NULL;
}

int main(int argc, const char *argv[])
{
    int status;
    pthread_t t1;
    pthread_t t2;
    pthread_t t3;

    status = pthread_barrier_init(&barrier, NULL, 3);
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
    pthread_barrier_destroy(&barrier); 
    return 0;
}

