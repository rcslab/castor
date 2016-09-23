
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <signal.h>
#include <execinfo.h>

#include <castor/debug.h>
#include <castor/rr_fdprintf.h>

#include "system.h"

#define MAX_LOG         512

void
Debug_LogBacktrace()
{
    const size_t MAX_FRAMES = 128;
    void *array[MAX_FRAMES];

    int num = backtrace(array, MAX_FRAMES);
    char **names = backtrace_symbols(array, num);
    for (int i = 0; i < num; i++) {
        LOG("%s", names[i]);
    }
    free(names);
}

void
Debug_PrintBacktrace()
{
    const size_t MAX_FRAMES = 128;
    void *array[MAX_FRAMES];

    int num = backtrace(array, MAX_FRAMES);
    char **names = backtrace_symbols(array, num);
    for (int i = 0; i < num; i++) {
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

    off = snprintf(buf, 32, "%016llx ", __builtin_readcyclecounter());

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

void Debug_Sighandler(int signum)
{
    const size_t MAX_FRAMES = 128;
    int num;
    void *array[MAX_FRAMES];
    char **names;

    Debug_Log(LEVEL_SYS, "Signal Caught: %d\n", signum);

    num = backtrace(array, MAX_FRAMES);
    names = backtrace_symbols(array, num);
    Debug_Log(LEVEL_SYS, "Backtrace:\n");
    for (int i = 0; i < num; i++) {
        if (names != NULL)
            Debug_Log(LEVEL_SYS, "[%d] %s\n", i, names[i]);
        else
            Debug_Log(LEVEL_SYS, "[%d] [0x%p]\n", i, array[i]);
    }
    free(names);

    abort();
}

int Debug_Init(const char *logPath) {
    signal(SIGSEGV, Debug_Sighandler);

    return 0;
}

