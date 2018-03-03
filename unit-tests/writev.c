#include <assert.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <string.h>
#include <unistd.h>

int main()
{
    ssize_t bytes_written;
    int fd = STDOUT_FILENO;
    char *buf0 = "short string\n";
    char *buf1 = "This is a longer string\n";
    char *buf2 = "This is the longest string in this example\n";
    int iovcnt;
    struct iovec iov[3];

    iov[0].iov_base = buf0;
    iov[0].iov_len = strlen(buf0);
    iov[1].iov_base = buf1;
    iov[1].iov_len = strlen(buf1);
    iov[2].iov_base = buf2;
    iov[2].iov_len = strlen(buf2);

    iovcnt = sizeof(iov) / sizeof(struct iovec);

    bytes_written = writev(fd, iov, iovcnt);
    assert(bytes_written == (ssize_t)(strlen(buf0) + strlen(buf1) + strlen(buf2)));
}
