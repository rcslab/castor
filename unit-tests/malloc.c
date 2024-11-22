#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#define STEP 100
#define SIZE_THRESH 4096

int main(int argc, const char *argv[])
{
    int i = 0, size;
    char *mem[STEP];
    
    srand(time(NULL));

    while (i++<32768) {
	size = rand() % SIZE_THRESH + 1; 
	printf("Allocating %d byte memory..\n", size);
	if (i % STEP == 0) {
		for (int j=0;j<STEP;j++) {
			free(mem[j]);
			mem[j] = NULL;
		}
	}
	mem[i%STEP] = malloc(size);
	printf("Allocated %d at 0x%x\n", size, mem[i%STEP]);
    }

    for (int j=0;j<STEP;j++) {
	free(mem[j]);
    }
}

