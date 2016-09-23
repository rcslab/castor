
#include <assert.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>

#include <castor/rr_debug.h>

int main(int argc, const char *argv[])
{
    struct stat sb;
    int fd;
    int len;
    char rr[1024];
    char norm[1024];

    fd = open("read.c", O_RDONLY);
    fstat(fd, &sb);
    len = read(fd, (void *)&rr, sb.st_size);
    assert(len == sb.st_size);

    lseek(fd, 0, SEEK_SET);
    if (rrMode != RRMODE_REPLAY) {
	len = syscall(SYS_read, fd, (void *)&norm, sb.st_size);
	assert(len == sb.st_size);

	for (int i = 0; i < sb.st_size; i++) {
	    if (rr[i] != norm[i]) {
		printf("Mismatch at %d\n", i);
	    }
	}
    }

    return 0;
}

