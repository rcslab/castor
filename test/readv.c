#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main()
{
    ssize_t bytes_read;
    char buf0[40];
    char buf1[40];

    int fd = open("readv-test.c", O_RDONLY);

    if (fd == -1) {
	perror("open failed");
    }

    struct iovec iov[3];

    iov[0].iov_base = buf0;
    iov[0].iov_len = sizeof(buf0);
    iov[1].iov_base = buf1;
    iov[1].iov_len = sizeof(buf1);

    bytes_read = readv(fd, iov, 2);

    for (int i = 0; i < 2; i++) {
	printf("%s \n", iov[i].iov_base);
    }

    return 0;
}

