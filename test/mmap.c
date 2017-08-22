#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

int main(int argc, const char *argv[])
{
    int fd;
    int status;
    struct stat sb;
    char *buf;

    // Test fd mmap
    fd = open("read.c", O_RDONLY);
    status = fstat(fd, &sb);
    assert(status == 0);

    buf = mmap(NULL, (size_t)sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(buf != MAP_FAILED);

    printf("Addr: %p\n", buf);
    printf("Data:");
    for (off_t i = 0; i < sb.st_size; i++) {
	if ((i % 32) == 0)
	    printf("\n");
	printf("%02x", buf[i]);
    }
    printf("\n");

    status = close(fd);
    assert(status == 0);

    status = munmap(buf, (size_t)sb.st_size);
    assert(status == 0);

    // Test anonymous mmap
    buf = mmap(NULL, 4*1024*1024, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    assert(buf != MAP_FAILED);
    status = munmap(buf, 4*1024*1024);
    assert(status == 0);

    return 0;
}

