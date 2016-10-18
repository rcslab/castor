
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <castor/debug.h>
#include <castor/archconfig.h>
#include <castor/rrshared.h>
#include <castor/Common/proc.h>
#include <castor/Common/runtime.h>
#include <castor/Common/cli.h>

void
usage()
{
    printf("replay [options] [program] [args]\n");
    printf("  -r [size]     Size of shared region\n");
    printf("  -e [entries]  Number of entries per thread\n");
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
    uintptr_t regionSz = RRLOG_DEFAULT_REGIONSZ;
    uint32_t numEvents = RRLOG_DEFAULT_EVENTS;
    int maxcpus = 64;
    bool ft = false;
    bool pinned = false;
    bool sandboxed = false;
    bool interactive = false;
    const char *logfile = "default.rr";

    while ((ch = getopt(argc, argv, "r:e:c:iho:ps")) != -1) {
	switch (ch) {
	    case 'r': {
		regionSz = strtoul(optarg, NULL, 10) * 1024*1024;
		break;
	    }
	    case 'e': {
		numEvents = strtoul(optarg, NULL, 10);
		break;
	    }
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
	OpenLog(logfile, regionSz, numEvents, false);
    }

    setenv("CASTOR_MODE", "REPLAY", 1);
    setenv("CASTOR_SHMPATH", logfile, 1);
    if (sandboxed) {
	setenv("CASTOR_SANDBOX", "1", 1);
    }
    if (Spawn(pinned, maxcpus, argv) < 0) {
	exit(1);
    }

    if (interactive) {
	CLI_Start();
    } else {
	ReplayLog();

	wait(&status);
	if (WIFSIGNALED(status)) {
	    WARNING("Child exited unexpectedly: %08x", WTERMSIG(status));
	    exit(1);
	}

	LogDone();


	// XXX: Handle forking programs better especially background ones

	return WEXITSTATUS(status);
    }

    return 0;
}

