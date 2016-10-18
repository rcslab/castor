
#include <assert.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, const char *argv[])
{
    pid_t pid = fork();

    assert(pid != -1);

    if (pid == 0) {
	printf("Child!\n");
    } else {
	printf("Parent!\n");
	wait(NULL);
    }
}

