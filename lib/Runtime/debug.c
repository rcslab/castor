
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <threads.h>

#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <execinfo.h>

#include <castor/debug.h>
#include <castor/events.h>
#include <castor/rrlog.h>
#include <castor/rrplay.h>
#include <castor/mtx.h>
#include <castor/rr_fdprintf.h>

#include "system.h"
#include "util.h"

#define MAX_LOG         512

void
Debug_LogBacktrace()
{
    const size_t MAX_FRAMES = 128;
    void *array[MAX_FRAMES];

    if (rrMode != RRMODE_NORMAL) {
	WARNING("Unable to print backtrace while in Record/Replay mode");
	return;
    }

    size_t num = backtrace(array, MAX_FRAMES);
    char **names = backtrace_symbols(array, num);
    for (size_t i = 0; i < num; i++) {
        LOG("%s", names[i]);
    }
    free(names);
}

void
Debug_PrintBacktrace()
{
    const size_t MAX_FRAMES = 128;
    void *array[MAX_FRAMES];

    if (rrMode != RRMODE_NORMAL) {
	WARNING("Unable to print backtrace while in Record/Replay mode");
	return;
    }

    size_t num = backtrace(array, MAX_FRAMES);
    char **names = backtrace_symbols(array, num);
    for (size_t i = 0; i < num; i++) {
        rr_fdprintf(STDERR_FILENO, "%s\n", names[i]);
    }
    free(names);
}


void
Debug_Log(int level, const char *fmt, ...)
{
    va_list ap;
    char buf[MAX_LOG];
    size_t off;

#if !defined(CASTOR_DEBUG)
    if (level > LEVEL_MSG)
        return;
#endif /* DEBUG */

    off = (size_t)rr_snprintf(buf, sizeof(buf), "%016llx %d ",
			      __builtin_readcyclecounter(),
			      SystemGetpid());

    switch (level) {
        case LEVEL_SYS:
            break;
        case LEVEL_ERR:
            strncat(buf, "ERROR: ", MAX_LOG - off);
            break;
        case LEVEL_WRN:
            strncat(buf, "WARNING: ", MAX_LOG - off);
            break;
        case LEVEL_MSG:
            strncat(buf, "MESSAGE: ", MAX_LOG - off);
            break;
        case LEVEL_LOG:
            strncat(buf, "LOG: ", MAX_LOG - off);
            break;
        case LEVEL_DBG:
            strncat(buf, "DEBUG: ", MAX_LOG - off);
            break;
        case LEVEL_VRB:
            strncat(buf, "VERBOSE: ", MAX_LOG - off);
            break;
    }

    off = strlen(buf);

    va_start(ap, fmt);
    rr_vsnprintf(buf + off, MAX_LOG - off, fmt, ap);
    va_end(ap);

    SystemWrite(STDERR_FILENO, buf, strlen(buf));
}

void Debug_LogHex(char *buf, size_t len)
{
    size_t off;
    char hbuf[128];
    size_t bufoff = 0;

    for (off = 0; off < len;) {
	for (size_t i = 0; i < 16 && off+i < len; i++) {
	    rr_snprintf(hbuf + bufoff, 128UL - bufoff, "%02X ", buf[off + i]);
	    bufoff += 3;
	}

	hbuf[bufoff] = '|';
	bufoff++;
	for (size_t i = 0; i < 16 && off < len; i++) {
	    rr_snprintf(hbuf + bufoff, 128UL - bufoff, "%c", buf[off]);
	    bufoff += 1;
	    off += 1;
	}
	hbuf[bufoff] = '|';
	bufoff++;

	Debug_Log(LEVEL_LOG, "%s\n", buf);
    }
}

void Debug_Sighandler(int signum)
{
    const size_t MAX_FRAMES = 128;
    void *array[MAX_FRAMES];

    Debug_Log(LEVEL_SYS, "Signal Caught: %d\n", signum);

    size_t num = backtrace(array, MAX_FRAMES);
    char **names = backtrace_symbols(array, num);
    Debug_Log(LEVEL_SYS, "Backtrace:\n");
    for (size_t i = 0; i < num; i++) {
	if (names != NULL)
	    Debug_Log(LEVEL_SYS, "[%zu] %s\n", i, names[i]);
	else
	    Debug_Log(LEVEL_SYS, "[%zu] [0x%p]\n", i, array[i]);
    }
    free(names);

    abort();
}

int Debug_Init(const char *logPath) {
    //signal(SIGSEGV, Debug_Sighandler);

    return 0;
}

