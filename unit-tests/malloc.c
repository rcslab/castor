#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define ALIGN_SIZE 16

int main(int argc, const char *argv[])
{
    int i = 1;
    char *mem;

    while (i<32768) {
	printf("Allocating %d byte memory..\n", i);
	mem = malloc(i);
	if (((unsigned long)mem % ALIGN_SIZE)!=0) {
	    printf("Address %p is not %d bytes aligned\n", mem, ALIGN_SIZE);
	    assert(0);
	}
	free(mem);
	i*=2;
    }
}

