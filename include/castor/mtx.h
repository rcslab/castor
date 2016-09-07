
#include <assert.h>
#include <stdint.h>
#include <stdalign.h>
#include <stdatomic.h>

#include <pthread.h>
#include <pthread_np.h>

#ifndef __CASTOR_MTX_H__
#define __CASTOR_MTX_H__

#include "archconfig.h"

typedef struct Mutex {
    alignas(CACHELINE) _Atomic(uint64_t)	lck;
    _Atomic(uint64_t)				thr;
} Mutex;

Mutex m;

static inline void
Mutex_Init(Mutex *m)
{
    atomic_store(&m->lck, 0);
    atomic_store(&m->thr, 0);
}

static inline void
Mutex_Lock(Mutex *m)
{
    uint64_t tid = pthread_getthreadid_np();

    assert(tid != 0);

    if (atomic_load(&m->thr) == tid) {
	atomic_fetch_add(&m->lck, 1);
	return;
    }

    uint64_t exp = 0;
    while (!atomic_compare_exchange_strong(&m->thr, &exp, tid)) {
	__asm__ volatile("pause\n");
	exp = 0;
    }
    atomic_fetch_add(&m->lck, 1);
}

static inline void
Mutex_Unlock(Mutex *m)
{
    uint64_t tid = pthread_getthreadid_np();

    assert(atomic_load(&m->lck) > 0);
    assert(atomic_load(&m->thr) == tid);

    if (atomic_fetch_sub(&m->lck, 1) == 1)
	atomic_store(&m->thr, 0);
}

#endif /* __CASTOR_MTX_H__ */

