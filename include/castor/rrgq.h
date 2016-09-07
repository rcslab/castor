
#ifndef __RRGQ_H__
#define __RRGQ_H__

#include <assert.h>
#include <stdint.h>
#include <stdalign.h>

#include "rrlog.h"

// Tunables
#define RRGQ_MAX_ENTRIES	(4096 * PAGESIZE / CACHELINE)

typedef struct RRGlobalQueue {
    alignas(CACHELINE) volatile uint64_t	head; // Next event to be written
    alignas(CACHELINE) volatile uint64_t	tail; // Oldest event to be written
    alignas(CACHELINE) volatile RRLogEntry	entries[RRGQ_MAX_ENTRIES];
} RRGlobalQueue;

static_assert(alignof(RRGlobalQueue) == CACHELINE, "RRGlobalQueue must be page aligned");

/*
 * Record/Replay Global Queue - Single-Writer Single-Reader Queue
 */
static inline void
RRGlobalQueue_Init(RRGlobalQueue *rrgq)
{
    assert((uintptr_t)rrgq % CACHELINE == 0);

    rrgq->head = 0;
    rrgq->tail = 0;
}

static inline void
RRGlobalQueue_Append(RRGlobalQueue *rrgq, RRLogEntry *rrentry)
{
    volatile RRLogEntry *e = &rrgq->entries[rrgq->head % RRGQ_MAX_ENTRIES];

    while ((rrgq->head - rrgq->tail) >= (RRGQ_MAX_ENTRIES - 1)) {
	pthread_yield();
	//__asm__ volatile("pause\n");
    }

    // Copy data
    e->eventId = rrentry->eventId;
    e->objectId = rrentry->objectId;
    e->event = rrentry->event;
    e->threadId = rrentry->threadId;
    e->value[0] = rrentry->value[0];
    e->value[1] = rrentry->value[1];
    e->value[2] = rrentry->value[2];
    e->value[3] = rrentry->value[3];
    e->value[4] = rrentry->value[4];

    rrgq->head += 1;
}

static inline RRLogEntry *
RRGlobalQueue_Dequeue(RRGlobalQueue *rrgq, uint64_t *numEntries)
{
    uint64_t head = rrgq->head;
    uint64_t maxContig;
    uint64_t maxEntries;

    maxContig = RRGQ_MAX_ENTRIES - (rrgq->tail % RRGQ_MAX_ENTRIES);
    maxEntries = head - rrgq->tail;

    if (maxContig < maxEntries) {
	*numEntries = maxContig;
    } else {
	*numEntries = maxEntries;
    }

    return (RRLogEntry *)&rrgq->entries[rrgq->tail % RRGQ_MAX_ENTRIES];
}

static inline void
RRGlobalQueue_Free(RRGlobalQueue *rrgq, uint64_t numEntries)
{
    rrgq->tail += numEntries;
}

static inline uint64_t
RRGlobalQueue_Length(RRGlobalQueue *rrgq)
{
    return (rrgq->head - rrgq->tail);
}

#endif /* __RRGQ_H__ */

