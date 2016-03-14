
#ifndef __RUNTIME_H__
#define __RUNTIME_H__

#include <threads.h>

enum RRMODE {
    RRMODE_NORMAL, // Normal
    RRMODE_RECORD, // Debug Recording
    RRMODE_REPLAY, // Replay
    RRMODE_FASTRECORD, // Fault Tolerance Recording
    RRMODE_FASTREPLAY, // Fault Tolerance Replay
};

extern enum RRMODE rrMode;
extern RRLog rrlog;
extern thread_local int threadId;

void LogDone();

#endif /* __RUNTIME_H__ */

