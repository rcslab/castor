
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc.h"
#include "runtime.h"
#include "cli.h"

void
usage()
{
    printf("replay [options] [program] [args]\n");
    printf("  -c [cores]    Maximum number of application cores\n");
    printf("  -i            Interactive debug shell\n");
    printf("  -o [log]      Log file\n");
    printf("  -p            Pin threads\n");
    printf("  -s            Sandbox\n");
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
    bool sandboxed = false;
    bool interactive = false;
    const char *logfile = "default.rr";

    while ((ch = getopt(argc, argv, "c:iho:ps")) != -1) {
	switch (ch) {
	    case 'c': {
		maxcpus = atoi(optarg);
		break;
	    }
	    case 'h': {
		usage();
		exit(0);
	    }
	    case 'i': {
		interactive = true;
		break;
	    }
	    case 'o': {
		logfile = optarg;
		break;
	    }
	    case 'p': {
		pinned = true;
		break;
	    }
	    case 's': {
		sandboxed = true;
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
	OpenLog(logfile, false);
    }

    setenv("CASTOR_MODE", "REPLAY", 1);
    if (sandboxed) {
	setenv("CASTOR_SANDBOX", "1", 1);
    }
    Spawn(pinned, maxcpus, argv);

    if (interactive) {
	CLI_Start();
    } else {
	ReplayLog();

	wait(&status);
	return WEXITSTATUS(status);
    }

    return 0;
}

