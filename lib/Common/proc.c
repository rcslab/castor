
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/param.h>
#include <sys/cpuset.h>
#include <sys/capsicum.h>

void
PinAgent()
{
    int status;
    cpuset_t mask;

    CPU_ZERO(&mask);
    CPU_SET(0, &mask);

    status = cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1, sizeof(mask), &mask);
    if (status != 0) {
	perror("cpuset_setaffinity");
	abort();
    }
}

void
PinProcess(int maxcpus)
{
    int status;
    cpuset_t all, mask;

    status = cpuset_getaffinity(CPU_LEVEL_ROOT, CPU_WHICH_PID, -1, sizeof(all), &all);
    if (status != 0) {
	perror("cpuset_getaffinity");
	abort();
    }

    CPU_ZERO(&mask);
    for (int i = 1; i < maxcpus; i++)
    {
	if (CPU_ISSET(i, &all)) {
	    CPU_SET(i, &mask);
	}
    }

    status = cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1, sizeof(mask), &mask);
    if (status != 0) {
	perror("cpuset_setaffinity");
	abort();
    }
}

int
Spawn(bool pinned, int maxcpus, char *const argv[])
{
    int pid;

    pid = fork();
    if (pid == -1) {
	perror("fork");
	abort();
    }
    if (pid != 0) {
	return pid;
    }

    if (pinned) {
	PinProcess(maxcpus);
    }

    execvp(*argv, argv);
    perror("execv");
    abort();

    return -1;
}

