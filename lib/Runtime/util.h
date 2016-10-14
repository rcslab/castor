
#ifndef __UTIL_H__
#define __UTIL_H__

#define LOCKTABLE_SIZE 4096

extern enum RRMODE rrMode;
extern RRLog *rrlog;
extern thread_local uint32_t threadId;
extern Mutex lockTable[LOCKTABLE_SIZE];

#if defined(CASTOR_DEBUG)
void AssertEvent(RRLogEntry *e, uint32_t evt);
void AssertReplay(RRLogEntry *e, bool test);
void AssertOutput(RRLogEntry *e, uint64_t hash, uint8_t *buf, size_t nbytes);
#elif defined(CASTOR_RELEASE)
#define AssertEvent(_e, _evt)
#define AssertReplay(_e, _tst)
#define AssertOutput(_e, _hash, _buf, _len)
#else
#error "Must define build type"
#endif

void logData(uint8_t *buf, size_t len);
uint64_t hashData(uint8_t *buf, size_t len);

#define GETLOCK(_obj) &lockTable[(_obj) % LOCKTABLE_SIZE]

static inline void
RRLog_LEnter(uint32_t threadId, uint64_t objId)
{
    RRLogEntry *e = RRLog_Alloc(rrlog, threadId);
    e->event = RREVENT_LOCKED_EVENT;
    e->objectId = objId;
    Mutex_Lock(GETLOCK(objId));
    RRLog_Append(rrlog, e);
}

static inline RRLogEntry *
RRLog_LAlloc(uint32_t threadId)
{
    return RRLog_Alloc(rrlog, threadId);
}

static inline void
RRLog_LAppend(RRLogEntry *entry)
{
    uint64_t objId = entry->objectId;

    RRLog_Append(rrlog, entry);
    Mutex_Unlock(GETLOCK(objId));
}

static inline void
RRPlay_LEnter(uint32_t threadId, uint64_t objId)
{
    RRLogEntry *e = RRPlay_Dequeue(rrlog, threadId);
    AssertEvent(e, RREVENT_LOCKED_EVENT);
    Mutex_Lock(GETLOCK(objId));
    RRPlay_Free(rrlog, e);
}

static inline RRLogEntry *
RRPlay_LDequeue(uint32_t threadId)
{
    return RRPlay_Dequeue(rrlog, threadId);
}

static inline void
RRPlay_LFree(RRLogEntry *entry)
{
    uint64_t objId = entry->objectId;

    RRPlay_Free(rrlog, entry);
    Mutex_Unlock(GETLOCK(objId));
}

static inline void
RRRecordI(uint32_t eventNum, int32_t result)
{
    RRLogEntry *e;

    e = RRLog_Alloc(rrlog, threadId);
    e->event = eventNum;
    e->objectId = 0;
    e->value[0] = result;
    if (result == -1) {
	e->value[1] = errno;
    }
    RRLog_Append(rrlog, e);
}

static inline void
RRReplayI(uint32_t eventNum, int32_t *result)
{
    RRLogEntry *e;

    e = RRPlay_Dequeue(rrlog, threadId);
    AssertEvent(e, eventNum);
    if (result != NULL) {
	*result =  e->value[0];
	if (*result == -1) {
	    errno = e->value[1];
	}
    }
    RRPlay_Free(rrlog, e);
}

static inline void
RRRecordOI(uint32_t eventNum, int od, int32_t result)
{
    RRLogEntry *e;

    e = RRLog_Alloc(rrlog, threadId);
    e->event = eventNum;
    e->objectId = od;
    e->value[0] = result;
    if (result == -1) {
	e->value[1] = errno;
    }
    RRLog_Append(rrlog, e);
}

static inline void
RRReplayOI(uint32_t eventNum, int *od, int32_t *result)
{
    RRLogEntry *e;

    e = RRPlay_Dequeue(rrlog, threadId);
    AssertEvent(e, eventNum);
    if (od != NULL) {
	*od = e->objectId;
    }
    if (result != NULL) {
	*result =  e->value[0];
	if (*result == -1) {
	    errno = e->value[1];
	}
    }
    RRPlay_Free(rrlog, e);
}

static inline void
RRRecordOU(uint32_t eventNum, int od, uint64_t result)
{
    RRLogEntry *e;

    e = RRLog_Alloc(rrlog, threadId);
    e->event = eventNum;
    e->objectId = od;
    e->value[0] = result;
    RRLog_Append(rrlog, e);
}

static inline void
RRReplayOU(uint32_t eventNum, int *od, uint64_t *result)
{
    RRLogEntry *e;

    e = RRPlay_Dequeue(rrlog, threadId);
    AssertEvent(e, eventNum);
    if (od != NULL) {
	*od = e->objectId;
    }
    if (result != NULL) {
	*result =  e->value[0];
    }
    RRPlay_Free(rrlog, e);
}

static inline void
RRRecordOS(uint32_t eventNum, int od, int64_t result)
{
    RRLogEntry *e;

    e = RRLog_Alloc(rrlog, threadId);
    e->event = eventNum;
    e->objectId = od;
    e->value[0] = result;
    if (result == -1) {
	e->value[1] = errno;
    }
    RRLog_Append(rrlog, e);
}

static inline void
RRReplayOS(uint32_t eventNum, int *od, int64_t * result)
{
    RRLogEntry *e;

    e = RRPlay_Dequeue(rrlog, threadId);
    AssertEvent(e, eventNum);
    if (od != NULL) {
	*od = e->objectId;
    }
    if (result != NULL) {
	*result = e->value[0];
	if (*result == -1) {
	    errno = e->value[1];
	}
    }
    RRPlay_Free(rrlog, e);
}



#endif /* __UTIL_H__ */

