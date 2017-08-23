
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/capsicum.h>

#include <castor/debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "system.h"
#include "util.h"

extern void Events_Init();
extern void Time_Init();

RRLog *rrlog;
enum RRMODE rrMode = RRMODE_NORMAL;
thread_local uint32_t threadId = 0; //-1;

__attribute__((constructor)) void
log_init()
{
    int status;

    Time_Init();

    char *shmpath = getenv("CASTOR_SHMPATH");
    if (shmpath == NULL) {
	//OldInit();
	return;
    }

    char *mode = getenv("CASTOR_MODE");
    if (mode == NULL) {
	rrMode = RRMODE_NORMAL;
	return;
    }

    Debug_Init("castor.log");

    key_t shmkey = ftok(shmpath, 0);
    if (shmkey == -1) {
	PERROR("ftok");
    }

    int shmid = shmget(shmkey, RRLOG_DEFAULT_REGIONSZ, 0);
    if (shmid == -1) {
	PERROR("shmget");
    }

    rrlog = shmat(shmid, NULL, 0);
    if (rrlog == MAP_FAILED) {
	PERROR("shmat");
    }

    RRShared_SetupThread(rrlog, 0);
    threadId = 0;

    // XXX: Need to remap the region again to the right size
    assert(RRLOG_DEFAULT_REGIONSZ == rrlog->regionSz);

    char *sandbox = getenv("CASTOR_SANDBOX");
    if (sandbox) {
	status = cap_enter();
	if (status != 0) {
	    PERROR("cap_enter");
	}
    }

    if (strcmp(mode, "RECORD") == 0) {
	fprintf(stderr, "RECORD\n");
	rrMode = RRMODE_RECORD;
    } else if (strcmp(mode, "REPLAY") == 0) {
	fprintf(stderr, "REPLAY\n");
	rrMode = RRMODE_REPLAY;
    }

    Events_Init();

    if (getenv("CASTOR_STOPCHILD")) {
        unsetenv("CASTOR_STOPCHILD");
        raise(SIGSTOP);
    }
}
