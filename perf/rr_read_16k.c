
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>

#include "stopwatch.h"

static int fd;

void
readBench(size_t rdsz)
{
    uint64_t start, stop;
    char buf[16*1024];

    start = Stopwatch_Start();
    for (int i = 0; i < (100*1024); i++)
    {
	read(fd, (void *)&buf, rdsz);
    }
    stop = Stopwatch_Stop();

    Stopwatch_Print(start, stop, 100*1024);
}

int main(int argc, const char *argv[])
{
    fd = open("/dev/zero", O_RDONLY);
    if (fd < 0) {
	perror("open");
	return 1;
    }

    readBench(16*1024);

    return 0;
}

