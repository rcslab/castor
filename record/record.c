
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include "proc.h"
#include "runtime.h"

void
usage()
{
    printf("record [options] [program] [args]\n");
    printf("  -c [cores]    Maximum number of application cores\n");
    printf("  -o [log]      Log file\n");
    printf("  -h            Help\n");
}

int
main(int argc, char *argv[])
{
    int ch;
    int maxcpus = 64;
    bool ft = false;
    const char *logfile;

    while ((ch = getopt(argc, argv, "c:ho:")) != -1) {
	switch (ch) {
	    case 'c': {
		maxcpus = atoi(optarg);
		break;
	    }
	    case 'h': {
		usage();
		exit(0);
	    }
	    case 'o': {
		logfile = optarg;
		break;
	    }
	    default: {
		usage();
		exit(1);
	    }
	}
    }
    argc -= optind;
    argv += optind;

    if (argc == 0) {
	usage();
	exit(1);
    }

    PinAgent();
    if (ft) {
	fprintf(stderr, "ft not supported");
	abort();
    } else {
	OpenLog(logfile, true);
    }

    setenv("CASTOR_MODE", "RECORD", 1);
    Spawn(maxcpus, argv);

    RecordLog();
}

