
#ifndef __UTIL_H__
#define __UTIL_H__

#include <threads.h>
#include <errno.h>

#define LOCKTABLE_SIZE 4096

extern thread_local enum RRMODE rrMode;
extern RRLog *rrlog;
extern Mutex lockTable[LOCKTABLE_SIZE];

#define BIND_REF(_name)\
    __strong_reference(__rr_ ## _name, _name); \
    __strong_reference(__rr_ ## _name, _ ## _name); \
    __strong_reference(__rr_ ## _name, __sys_ ## _name)

void dumpLog();
#if defined(CASTOR_DEBUG)
void AssertObject(RRLogEntry *e, uint64_t od);
void AssertEvent(RRLogEntry *e, uint32_t evt);
void AssertReplay(RRLogEntry *e, bool test);
void AssertOutput(RRLogEntry *e, uint64_t hash, uint8_t *buf, size_t nbytes);
#elif defined(CASTOR_RELEASE)
#define AssertObject(_e, _od)
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
    RRLogEntry *e = RRLog_Alloc(rrlog, getThreadId());
    e->event = RREVENT_LOCKED_EVENT;
    e->objectId = objId;
    e->value[4] = __builtin_readcyclecounter();
    Mutex_Lock(GETLOCK(objId));
    RRLog_Append(rrlog, e);
}

static inline RRLogEntry *
RRLog_LAlloc(uint32_t threadId)
{
    return RRLog_Alloc(rrlog, getThreadId());
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
    RRLogEntry *e = RRPlay_Dequeue(rrlog, getThreadId());
    AssertEvent(e, RREVENT_LOCKED_EVENT);
    Mutex_Lock(GETLOCK(objId));
    RRPlay_Free(rrlog, e);
}

static inline RRLogEntry *
RRPlay_LDequeue(uint32_t threadId)
{
    return RRPlay_Dequeue(rrlog, getThreadId());
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

    e = RRLog_Alloc(rrlog, getThreadId());
    e->event = eventNum;
    e->objectId = 0;
    e->value[0] = (uint64_t)result;
    if (result == -1) {
	e->value[1] = (uint64_t)errno;
    }
    RRLog_Append(rrlog, e);
}

static inline void
RRRecordOI(uint32_t eventNum, int od, int32_t result)
{
    RRLogEntry *e;

    e = RRLog_Alloc(rrlog, getThreadId());
    e->event = eventNum;
    e->objectId = (uint64_t)od;
    e->value[0] = (uint64_t)result;
    if (result == -1) {
	e->value[1] = (uint64_t)errno;
    }
    RRLog_Append(rrlog, e);
}

static inline void
RRReplayI(uint32_t eventNum, int32_t *result)
{
    RRLogEntry *e;

    e = RRPlay_Dequeue(rrlog, getThreadId());
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
RRReplayOI(uint32_t eventNum, int od, int32_t *result)
{
    RRLogEntry *e;

    e = RRPlay_Dequeue(rrlog, getThreadId());
    AssertEvent(e, eventNum);
    AssertObject(e, (uint64_t)od);
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

    e = RRLog_Alloc(rrlog, getThreadId());
    e->event = eventNum;
    e->objectId = (uint64_t)od;
    e->value[0] = (uint64_t)result;
    RRLog_Append(rrlog, e);
}

static inline void
RRReplayOU(uint32_t eventNum, int od, uint64_t *result)
{
    RRLogEntry *e;

    e = RRPlay_Dequeue(rrlog, getThreadId());
    AssertEvent(e, eventNum);
    AssertObject(e, (uint64_t)od);
    if (result != NULL) {
	*result =  e->value[0];
    }
    RRPlay_Free(rrlog, e);
}

static inline void
RRRecordS(uint32_t eventNum, int64_t result)
{
    RRLogEntry *e;

    e = RRLog_Alloc(rrlog, getThreadId());
    e->event = eventNum;
    e->value[0] = (uint64_t)result;
    if (result == -1) {
	e->value[1] = (uint64_t)errno;
    }
    RRLog_Append(rrlog, e);
}

static inline void
RRReplayS(uint32_t eventNum, int64_t * result)
{
    RRLogEntry *e;

    e = RRPlay_Dequeue(rrlog, getThreadId());
    AssertEvent(e, eventNum);
    if (result != NULL) {
	*result = (int64_t)e->value[0];
	if (*result == -1) {
	    errno = (int64_t)e->value[1];
	}
    }
    RRPlay_Free(rrlog, e);
}

static inline void
RRRecordOS(uint32_t eventNum, int od, int64_t result)
{
    RRLogEntry *e;

    e = RRLog_Alloc(rrlog, getThreadId());
    e->event = eventNum;
    e->objectId = (uint64_t)od;
    e->value[0] = (uint64_t)result;
    if (result == -1) {
	e->value[1] = (uint64_t)errno;
    }
    RRLog_Append(rrlog, e);
}

static inline void
RRReplayOS(uint32_t eventNum, int od, int64_t * result)
{
    RRLogEntry *e;

    e = RRPlay_Dequeue(rrlog, getThreadId());
    AssertEvent(e, eventNum);
    AssertObject(e, (uint64_t)od);
    if (result != NULL) {
	*result = (int64_t)e->value[0];
	if (*result == -1) {
	    errno = (int64_t)e->value[1];
	}
    }
    RRPlay_Free(rrlog, e);
}

#endif /* __UTIL_H__ */
