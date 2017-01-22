
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <castor/debug.h>
#include <castor/archconfig.h>
#include <castor/rrshared.h>
#include <castor/Common/proc.h>
#include <castor/Common/runtime.h>
#include <castor/Common/ft.h>

void
usage()
{
    printf("record [options] [program] [args]\n");
    printf("  -m	    Master\n");
    printf("  -s [hostname] Slave\n");
    printf("  -r [size]     Size of shared region\n");
    printf("  -e [entries]  Number of entries per thread\n");
    printf("  -c [cores]    Maximum number of application cores\n");
    printf("  -p            Pin threads\n");
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
    bool pinned = false;
    bool master = true;
    char *host;
    char logfile[64];

    while ((ch = getopt(argc, argv, "ms:r:e:c:ph")) != -1) {
	switch (ch) {
	    case 'm': {
		master = true;
		break;
	    }
	    case 's': {
		master = false;
		host = optarg;
		break;
	    }
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

    sprintf(&logfile[0], "castor-%d-%d.log", (int)getpid(), (int)master);

    OpenLog(logfile, regionSz, numEvents, true);
    setenv("CASTOR_MODE", "RECORD", 1);
    setenv("CASTOR_SHMPATH", logfile, 1);

    if (master) {
	setenv("CASTOR_MODE", "RECORD", 1);
	RRFT_InitMaster();
    } else {
	setenv("CASTOR_MODE", "REPLAY", 1);
	RRFT_InitSlave(host);
    }

    int pid = Spawn(pinned, maxcpus, argv);
    if (pid < 0) {
	exit(1);
    }

    setproctitle("Castor pid=%d log=%s", pid, logfile);

    if (master) {
	RecordLog();
    } else {
	ReplayLog();
    }

    while (1) {
	pid_t p = wait(&status);
	if (p == -1) {
	    if (errno == ECHILD)
		break;
	    PERROR("wait");
	}
	if (WIFSIGNALED(status)) {
	    WARNING("Child exited unexpectedly: %08x", WTERMSIG(status));
	    DumpLogDebug();
	    exit(1);
	}
    }

    LogDone();

    return WEXITSTATUS(status);
}

