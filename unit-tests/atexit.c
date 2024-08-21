#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void 
bye(void)
{
    printf("ok bye. My uid is %u\n", getuid());
}

int
main(int argc, const char *argv[])
{
    if (atexit(bye) != 0) {
	perror("atexit");
	exit(0);
    }
    printf("Hello World!\n");

    return 0;
}

