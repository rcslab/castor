
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/cpuset.h>
#include <sys/capsicum.h>

#include <castor/debug.h>

void
PinAgent()
{
    int status;
    cpuset_t mask;

    CPU_ZERO(&mask);
    CPU_SET(0, &mask);

    status = cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1, sizeof(mask), &mask);
    if (status != 0) {
	PERROR("cpuset_setaffinity");
    }
}

void
PinProcess(int maxcpus)
{
    int status;
    cpuset_t all, mask;

    status = cpuset_getaffinity(CPU_LEVEL_ROOT, CPU_WHICH_PID, -1, sizeof(all), &all);
    if (status != 0) {
	PERROR("cpuset_getaffinity");
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
	PERROR("cpuset_setaffinity");
    }
}

int
Spawn(bool pinned, int maxcpus, char *const argv[])
{
    int pid;

    pid = fork();
    if (pid == -1) {
	PERROR("fork");
    }
    if (pid != 0) {
	return pid;
    }

    if (pinned) {
	PinProcess(maxcpus);
    }

    execvp(*argv, argv);

    // PERROR("execv");
	char pstr[64];
	strerror_r(errno, &pstr[0], sizeof(pstr));
	Debug_Log(LEVEL_SYS, "execvp: %s\n", pstr);
	abort();

    return -1;
}

