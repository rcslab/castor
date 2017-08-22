#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

int main()
{
    int fds1[2];
    int fds2[2];

    pid_t pid;
    char msg[] = "once upon a midnight dreary";
    char msg2[] = "as I pondered weak and weary";

    if (pipe(fds1) != 0) {
	perror("pipe");
	exit(1);
    }
    assert(pipe2(fds2, O_CLOEXEC) == 0);

    pid = fork();

    if (pid < 0) {
	perror("fork");
	exit(1);
    } else if (pid == 0) {
	close(fds1[0]);
	close(fds2[0]);

	assert(write(fds1[1], msg, strlen(msg) + 1) > 0);
	assert(write(fds2[1], msg2, strlen(msg2) + 1) > 0);
	return 0;

    } else {
	char buff[128];
	close(fds1[1]);
	close(fds2[1]);

	assert(read(fds1[0], buff, sizeof(buff)) > 0);
	printf("%s\n", buff);

	bzero(buff, sizeof(buff));
	assert(read(fds2[0], buff, sizeof(buff)) > 0);
	printf("%s\n", buff);
    }
}


