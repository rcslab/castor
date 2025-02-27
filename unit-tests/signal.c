
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <semaphore.h>

sem_t ready;

void 
signal_handler(int sig) 
{
    printf("Child: received signal %d\n", sig);
    if (sig == SIGUSR1) {
	sem_post(&ready);
    }
}

int
main()
{
    pid_t pid;

    pid = fork();
    if (pid == 0) {
	sem_init(&ready, 0, 0);

	printf("Child: running..\n");
	signal(SIGUSR1, signal_handler);
	sem_wait(&ready);
	printf("Child: bye!\n");

	sem_destroy(&ready);
    } else {
	printf("Main: child pid is %d\n", pid);
	sleep(1);
	kill(pid, SIGUSR1);
	waitpid(pid, 0, 0);
	printf("Main: bye!\n");
    }
}
