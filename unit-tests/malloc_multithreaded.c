#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include <pthread.h>

#define TOTAL_ITER 3 //512
#define STEP 2 //16
#define SIZE_THRESH 4096
#define THREAD_COUNT 8

void *
routine(void *arg) {
    int i = 0, size;
    char *mem[STEP];
    intptr_t thr_no = (intptr_t)arg;

    printf("Thread %ld started..\n", thr_no);

    for (i=0;i<TOTAL_ITER;i++) {
	size = rand() % SIZE_THRESH + 1; 
	if (i!=0 && i%STEP==0) {
	    for (int j=0;j<STEP;j++) {
	    	printf("[%ld] Free 0x%p..\n", thr_no, mem[j]);
	   	free(mem[j]);
	    	mem[j] = NULL;
	    	printf("[%ld] Freed.\n", thr_no);
	    }
	}
	mem[i%STEP] = malloc(size);
	printf("[%ld] Allocated %d byte memory @ 0x%p..\n", thr_no, size, mem[i%STEP]);
    }

    for (int j=0;j<STEP;j++) {
	if (mem[j]) {
	    free(mem[j]);
	    printf("[%ld] Freed 0x%p..\n", thr_no, mem[j]);
	}
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

