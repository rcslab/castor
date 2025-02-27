#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h> 

int main(int argc, char *argv[]) {
    int c_arg = 0;
    char *c_argv[24];

    printf("hello\n");

    if (argc > 1) {
	printf("execv done\n");
    } else {
	c_argv[c_arg++] = (char *)argv[0];
	c_argv[c_arg++] = "1";
	c_argv[c_arg++] = NULL;
	execv(c_argv[0], c_argv);
	assert(0);
    }
}


