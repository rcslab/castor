
#ifndef __RUNTIME_H__
#define __RUNTIME_H__

#include <threads.h>

enum RRMODE {
    RRMODE_NORMAL, // Normal
    RRMODE_RECORD, // Debug Recording
    RRMODE_REPLAY, // Replay
    RRMODE_FDREPLAY, // Replay (Real Files)
    RRMODE_FTREPLAY, // Replay (Fault Tolerance)
};

extern enum RRMODE rrMode;
extern RRLog *rrlog;
extern thread_local int threadId;

void LogDone();

#endif /* __RUNTIME_H__ */

