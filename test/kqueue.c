
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

int main(int argc, const char *argv[])
{
    int fd = kqueue();
    int status;

    status = kevent(fd, NULL, 0, NULL, 0, NULL);
    printf("status %d\n", status);

    close(fd);

    return 0;
}

