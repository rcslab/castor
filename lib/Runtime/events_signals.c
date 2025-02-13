
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

#include <stdatomic.h>
#include <threads.h>

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/cdefs.h>
#include <sys/syscall.h>

#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <libc_private.h>

#include <castor/debug.h>
#include <castor/rrshared.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/rrgq.h>
#include <castor/mtx.h>
#include <castor/events.h>

#include "system.h"
#include "util.h"

struct rr_signal_entry {
    union { 
	void (*handler)(int);
	void (*sigaction)(int, siginfo_t *, void *);
    };
};

static struct rr_signal_entry rr_signal_table[SIGRTMAX+1]; 

void
rr_sigaction_handler(int sig, siginfo_t *info, void *ctx) 
{
    switch (rrMode) {
    case RRMODE_NORMAL:
	/* Do nothing */
	break;
    case RRMODE_RECORD:
    case RRMODE_REPLAY:
	LOG("Signal %d received.", sig);
	if (rr_signal_table[sig].handler) {
	    if (info || ctx) {
		rr_signal_table[sig].sigaction(sig, info, ctx);
	    } else {
		rr_signal_table[sig].handler(sig);
	    }
	}
	
	break;
    }
}

void 
rr_signal_handler(int sig)
{
    rr_sigaction_handler(sig, NULL, NULL);
}

int
__rr_sigaction(int sig, const struct sigaction *restrict act, 
    struct sigaction *restrict oact) 
{
    struct sigaction rr_act, *in = NULL;
    void (*sah)(int);
    int status = 0;

    switch (rrMode) {
    case RRMODE_NORMAL:
	status = __rr_syscall(SYS_sigaction, sig, act, oact);
	break;
    case RRMODE_RECORD:
    case RRMODE_REPLAY:
	sah = act->sa_handler;

	if (act) {
	    memcpy(&rr_act, act, sizeof(struct sigaction));
	    rr_act.sa_handler = &rr_signal_handler;
	    in = &rr_act;
	}

	status = __rr_syscall(SYS_sigaction, sig, in, oact);
	if (status == 0) {
	    if (oact) {
		oact->sa_handler = rr_signal_table[sig].handler;
	    }
	    if (act) {
		rr_signal_table[sig].handler = sah;
	    }
	}
	break;
    }

    return status;
}

/*
 * TODO: simulate the error case in kill
 *
 */
int 
__rr_kill(pid_t pid, int sig) 
{
    int status;
    pid_t realpid;

    switch (rrMode) {
    case RRMODE_NORMAL:
    case RRMODE_RECORD:
	status = __rr_syscall(SYS_kill, pid, sig);
	break;
    case RRMODE_REPLAY:
	// Replace the recorded pid with the real pid
	realpid = RRShared_FindRealPidFromRecordedPid(rrlog, pid);
	status = __rr_syscall(SYS_kill, realpid, sig);
	break;
    }

    return status;
}

BIND_REF(kill);
BIND_REF(sigaction);

