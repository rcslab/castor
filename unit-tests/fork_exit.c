
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
	printf("A fork B\n");
	if (fork() != 0) {
		printf("A exit\n");
		exit(0);
	} else {
		usleep(300);
		printf("B fork C\n");
		if (fork() != 0) {
		    usleep(300);
		    printf("B exit\n");
		    exit(0);
		} else {
		    printf("C exit\n");
		}
	}
}
