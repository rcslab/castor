#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h> 

int main(int argc, char *argv[]) {
    int c_arg, status;
    char *c_argv[24];

    printf("hello\n");

    if (argc > 1) {
	printf("execv done\n");
    } else {
	c_argv[c_arg++] = "./execv";
        c_argv[c_arg++] = "1";
        c_argv[c_arg++] = NULL;
        status = execv(c_argv[0], c_argv);
	assert(status == -1);
    }
}


