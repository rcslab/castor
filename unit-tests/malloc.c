#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#define STEP 100
#define SIZE_THRESH 4096

int main(int argc, const char *argv[])
{
    int i = 0, size;
    char *mem[STEP];
    
    srand(time(NULL));
    memset(mem, 0, sizeof(mem));

    while (i++<32768) {
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
}

