
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <castor/debug.h>
#include <castor/archconfig.h>
#include <castor/rrshared.h>
#include <castor/Common/proc.h>
#include <castor/Common/runtime.h>

void
usage()
{
    printf("record [options] [program] [args]\n");
    printf("  -r [size]     Size of shared region\n");
    printf("  -e [entries]  Number of entries per thread\n");
    printf("  -c [cores]    Maximum number of application cores\n");
    printf("  -o [log]      Log file\n");
    printf("  -p            Pin threads\n");
    printf("  -t            Stops child process on creation for attaching a debugger in record mode\n");
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
    bool stopchild = false;
    const char *logfile = "default.rr";

    while ((ch = getopt(argc, argv, "r:e:c:ho:pt")) != -1) {
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
	    case 'o': {
		logfile = optarg;
		break;
	    }
	    case 'p': {
		pinned = true;
		break;
	    }
        case 't' : {
        stopchild = true;
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
	OpenLog(logfile, regionSz, numEvents, true);
    }

    setenv("CASTOR_MODE", "RECORD", 1);
    setenv("CASTOR_SHMPATH", logfile, 1);
    if (stopchild) setenv("CASTOR_STOPCHILD", "1", 1);
    
    int pid = Spawn(pinned, maxcpus, argv);
    if (pid < 0) {
	exit(1);
    }

    setproctitle("Castor pid=%d log=%s", pid, logfile);
    
    RecordLog();

    while (1) {
	pid_t p = wait(&status);
	if (p == -1) {
	    if (errno == ECHILD)
		break;
	    PERROR("wait");
	}
	if (WIFSIGNALED(status)) {
	    int signum = WTERMSIG(status);
	    WARNING("Child exited unexpectedly, recieved: SIG%s - %s",
		    sys_signame[signum], strsignal(signum));
	    exit(1);
	}
    }

    LogDone();

    return WEXITSTATUS(status);
}

