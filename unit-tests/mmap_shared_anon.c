
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <castor/rr_debug.h>

#define TESTSIZE (1024*1024)

int main(int argc, const char *argv[])
{
    int status;
    void *buf;
    atomic_int *i;
    pid_t p;

    buf = mmap(NULL, (size_t)TESTSIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);
    assert(buf != MAP_FAILED);

    i = (atomic_int *)buf;
    printf("Addr: %p\n", buf);

    p = fork();
    assert(p != -1);
    
    if (p == 0) { // Child
	while (atomic_load(i) == 0) {
	    sleep(0);
	}
	printf("Recieved\n");
    } else { // Parent
	atomic_store(i, 1);
	printf("Sent\n");
    }

    status = munmap(buf, (size_t)TESTSIZE);
    assert(status == 0);

    if (p != 0) {
	wait(NULL);
    }

    return 0;
}

