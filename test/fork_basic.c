
#include <assert.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

int main(int argc, const char *argv[])
{
    pid_t pid;

    assert(wait(NULL) == -1);
    assert(errno == ECHILD);

    pid = fork();
    assert(pid != -1);

    if (pid == 0) {
	printf("Child %d!\n", getpid());
    } else {
	int status;
	pid_t cpid;

	printf("Parent %d!\n", getpid());
	cpid = wait(&status);
	assert(cpid == pid);
	printf("Child Wait: %d Status: %d\n", cpid, status);
    }
}

