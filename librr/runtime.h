
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

int RRMutex_Init(pthread_mutex_t *mtx, pthread_mutexattr_t *attr);
int RRMutex_Destroy(pthread_mutex_t *mtx);
int RRMutex_Lock(pthread_mutex_t *mtx);
int RRMutex_TryLock(pthread_mutex_t *mtx);
int RRMutex_Unlock(pthread_mutex_t *mtx);

#endif /* __RUNTIME_H__ */

