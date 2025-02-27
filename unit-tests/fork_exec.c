
#include <assert.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>

#include <castor/rrshared.h>

int main(int argc, const char *argv[])
{
	pid_t pid;
	int status;
	int no_fork = 0;

	if (argc > 1) {
	    printf("Last fork start.\n");
	    no_fork = 1;
	    pid = getpid();
	}
		
	if (!no_fork) {
	    pid = fork();
	    assert(pid != -1);
	} 

	if (pid == 0) {
	    /* Child */
	    char *c_argv[24];
	    int c_arg = 0;
	    c_argv[c_arg++] = (char *)argv[0];
	    c_argv[c_arg++] = "1";
	    c_argv[c_arg++] = NULL;

	    printf("execv..\n");
	    assert(getThreadId() != 0);
	    status = execv(c_argv[0], c_argv);
	    printf("execv status %d, errno %u\n", status, errno);
	    assert(status != -1);
	} else {
	    int status;
	    /*
	     * If the execv resets the threadid (to 0) in Castor, then if the 
	     * main thread/parent, whose threadid is 0, keeping calling random
	     * syscalls here, the child cannot append the log and breaks the log. 
	     */
	    if (!no_fork) {
		assert(getThreadId() == 0);

		printf("Waiting for child..\n");
		wait(&status);
		printf("Bye.\n");
	    } else {
		assert(getThreadId() != 0);
		printf("Last fork done.\n");
	    }
	}
}
