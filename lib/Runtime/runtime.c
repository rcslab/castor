
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdlib.h>

#include <threads.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/capsicum.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/shm.h>

#include <castor/debug.h>
#include <castor/rr_debug.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "system.h"
#include "util.h"

extern char * __castor_getenv(const char * name);

RRLog *rrlog;
enum RRMODE rrMode = RRMODE_NORMAL;

__attribute__((constructor)) void
log_init()
{
    int status;

    char *shmpath = __castor_getenv("CASTOR_SHMPATH");
    if (shmpath == NULL) {
	return;
    }

    char *mode = __castor_getenv("CASTOR_MODE");
    if (mode == NULL) {
	rrMode = RRMODE_NORMAL;
	return;
    }

    Debug_Init("castor.log");

    LOG("shmpath = %s", shmpath);
    LOG("mode = %s", mode);

    key_t shmkey = __castor_ftok(shmpath, 0);
    if (shmkey == -1) {
	PERROR("ftok");
    }

    LOG("shmkey = %ld", shmkey);

    int shmid = __rr_syscall(SYS_shmget, shmkey, RRLOG_DEFAULT_REGIONSZ, 0);
    if (shmid == -1) {
	PERROR("shmget");
    }

    rrlog = (RRLog *) __rr_syscall_long(SYS_shmat, shmid, NULL, 0);
    if (rrlog == MAP_FAILED) {
	PERROR("shmat");
    }

    RRShared_SetupThread(rrlog, 0);
    setThreadId(0);

    // XXX: Need to remap the region again to the right size
    rr_assert(RRLOG_DEFAULT_REGIONSZ == rrlog->regionSz);

    char *sandbox = __castor_getenv("CASTOR_SANDBOX");
    if (sandbox) {
	status = __rr_syscall(SYS_cap_enter);
	if (status != 0) {
	    PERROR("cap_enter");
	}
    }

    char *stopchild = __castor_getenv("CASTOR_STOPCHILD");
    if (stopchild && strcmp(stopchild,"1") == 0) {
	while (atomic_load(&rrlog->dbgwait) != 1)
	    ;
    }

    if (strcmp(mode, "RECORD") == 0) {
	rr_fdprintf(STDERR_FILENO, "RECORD\n");
	rrMode = RRMODE_RECORD;
    } else if (strcmp(mode, "REPLAY") == 0) {
	rr_fdprintf(STDERR_FILENO, "REPLAY\n");
	rrMode = RRMODE_REPLAY;
    }
}
