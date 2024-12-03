
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>

int done = 0;

void 
signal_handler(int sig) 
{
    printf("Child: received signal %d\n", sig);
    if (sig == SIGUSR1) {
	done = 1;
    }
}

int
main()
{
    pid_t pid;

    pid = fork();
    if (pid == 0) {
	signal(SIGUSR1, signal_handler);
	while (done != 1) {
	    sleep(1);
	}
	printf("Child: bye!\n");
    } else {
	printf("Main: child pid is %d\n", pid);
	sleep(1);
	kill(pid, SIGUSR1);
	waitpid(pid, 0, 0);
	printf("Main: bye!\n");
    }
}
