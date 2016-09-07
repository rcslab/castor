
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <castor/Common/proc.h>
#include <castor/Common/runtime.h>

void
usage()
{
    printf("record [options] [program] [args]\n");
    printf("  -c [cores]    Maximum number of application cores\n");
    printf("  -o [log]      Log file\n");
    printf("  -p            Pin threads\n");
    printf("  -h            Help\n");
}

int
main(int argc, char *argv[])
{
    int status;
    int ch;
    int maxcpus = 64;
    bool ft = false;
    bool pinned = false;
    const char *logfile = "default.rr";

    while ((ch = getopt(argc, argv, "c:ho:p")) != -1) {
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
	    case 'p': {
		pinned = true;
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

    if (pinned) {
	PinAgent();
    }
    if (ft) {
	fprintf(stderr, "ft not supported");
	abort();
    } else {
	OpenLog(logfile, true);
    }

    setenv("CASTOR_MODE", "RECORD", 1);
    Spawn(pinned, maxcpus, argv);

    RecordLog();

    wait(&status);
    return WEXITSTATUS(status);
}

