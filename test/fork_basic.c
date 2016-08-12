
#include <stdio.h>

#include <unistd.h>

int main(int argc, const char *argv[])
{
    pid_t pid = fork();

    if (pid == 0)
	printf("Child!\n");
    else
	printf("Parent!\n");
}

