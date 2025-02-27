#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>

#include <pthread.h>

#define TOTAL_ITER 512
#define STEP 10
#define SIZE_THRESH 4096
#define THREAD_COUNT 8

void *
routine(void *arg) {
    int i = 0, size;
    char *mem[STEP];
    intptr_t thr_no = (intptr_t)arg;

    printf("Thread %ld started..\n", thr_no);

    while (i++<TOTAL_ITER) {
	size = rand() % SIZE_THRESH + 1; 
	
	printf("[%d] Allocating %d byte memory..\n", i, size);
	if (i % STEP == 0) {
		printf("Cleaning..\n");
		for (int j=0;j<STEP;j++) {
			free(mem[j]);
		}
		memset(mem, 0, sizeof(mem));
	}
	mem[i%STEP] = malloc(size);
	printf("Allocated %d at %p\n", size, mem[i%STEP]);
    }

    for (int j=0;j<STEP;j++) {
	free(mem[j]);
    }

    printf("Thread %ld exiting..\n", thr_no);
    return NULL;
}

int main(int argc, const char *argv[])
{
    int status;
    pthread_t thrs[THREAD_COUNT];
   
    assert(TOTAL_ITER >= STEP);
    srand(time(NULL));

    for (int i=0;i<THREAD_COUNT;i++) {
	status = pthread_create(&(thrs[i]), NULL, &routine, (void *)(long)i);
	assert(status == 0);
    }

    for (int i=0;i<THREAD_COUNT;i++) {
	pthread_join(thrs[i], NULL);
    }
}

