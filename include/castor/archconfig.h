
#ifndef __CASTOR_ARCHCONFIG_H__
#define __CASTOR_ARCHCONFIG_H__

#include <assert.h>
#include <stdalign.h>
#include <stdint.h>

#ifdef USE_L3CACHELINE
#define CACHELINE	128
#else
#define CACHELINE	64
#endif

#define PAGESIZE	4096

// Castor Tunables
#define RRLOG_MAX_THREADS	8
#if defined(CASTOR_FT)
#define RRLOG_MAX_ENTRIES	(16*PAGESIZE / CACHELINE - 4)
#elif defined(CASTOR_DBG)
#define RRLOG_MAX_ENTRIES	(1024*PAGESIZE / CACHELINE - 4)
#elif defined(CASTOR_SNAP)
#define RRLOG_MAX_ENTRIES	(128*1024*PAGESIZE / CACHELINE - 4)
#else
#error "Unknown logging configuration"
#endif

enum RRMODE {
    RRMODE_NORMAL, // Normal
    RRMODE_RECORD, // Debug Recording
    RRMODE_REPLAY, // Replay
    //RRMODE_FASTRECORD, // Fault Tolerance Recording
    //RRMODE_FASTREPLAY, // Fault Tolerance Replay
};

typedef struct RRLogEntry {
    alignas(CACHELINE) uint64_t		eventId;
    uint64_t				objectId;
    uint32_t				event;
    uint32_t				threadId;
    uint64_t				value[5];
} RRLogEntry;

static_assert(alignof(RRLogEntry) == CACHELINE, "RRLogEntry must be cache line aligned");
static_assert(sizeof(RRLogEntry) == CACHELINE, "RRLogEntry must be cache line sized");

typedef struct RRLogThread {
    alignas(CACHELINE) volatile uint64_t	freeOff; // Next free entry
    alignas(CACHELINE) volatile uint64_t	usedOff; // Next entry to consume
    alignas(CACHELINE) volatile uint64_t	status; // Debugging
    alignas(CACHELINE) volatile uint64_t	_rsvd;
    alignas(CACHELINE) volatile RRLogEntry	entries[RRLOG_MAX_ENTRIES];
} RRLogThread;

static_assert(alignof(RRLogThread) == CACHELINE, "RRLogThread must be page aligned");

typedef struct RRLog {
    alignas(CACHELINE) volatile uint64_t nextEvent; // Next event to be read
    alignas(CACHELINE) volatile uint64_t lastEvent; // Last event to be allocated
    alignas(PAGESIZE) RRLogThread threads[RRLOG_MAX_THREADS];
} RRLog;

static_assert(alignof(RRLog) == PAGESIZE, "RRLog must be page aligned");

#endif /* __CASTOR_ARCHCONFIG_H__ */

