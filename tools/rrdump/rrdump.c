
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <castor/debug.h>
#include <castor/archconfig.h>
#include <castor/rrshared.h>

void
usage()
{
    printf("rrdump [coredump]\n");
}

int
main(int argc, char *argv[])
{
//    int status;

    if (argc == 0) {
	usage();
	exit(1);
    }

    // Open coredump

    // Find RRLog

    // Dump RRLog

    // Dump RRThread

    // Dump other

    return 0;
}

