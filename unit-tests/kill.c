/*
 * The KILL test case is not working currently.
 *  1. The signal delivery is not fully working yet.
 *  2. The SIGINT/SIGTERM/SIGHUP signal leads to the target application quits
 *  without generating RREVENT_EXIT, which causes the rrthr/gqthr to loop 
 *  infinitely.
 */

#include <assert.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

int main(int argc, const char *argv[])
{
    pid_t pid;

    assert(wait(NULL) == -1);
    assert(errno == ECHILD);

    pid = fork();
    assert(pid != -1);

    if (pid == 0) {
	printf("Child %d!\n", getpid());
	printf("Parent of child %d!\n", getppid());
	while (1) { }
	assert(0);
    } else {
	sleep(1);
	printf("Killing child pid %d..\n", pid);
	if (kill(pid, SIGINT) == 0) {
	    printf("Child killed.\n");
	} else {
	    perror("Kill: ");
	    assert(0);
	}
    }
}
