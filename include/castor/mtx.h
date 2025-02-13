
#ifndef __CASTOR_MTX_H__
#define __CASTOR_MTX_H__

/*
 * TODO: For now it seems like we don't need this, otherwise we'll need to 
 * replace pthread_getthreadid_np() with getThreadId() and make sure it works 
 * throughout thread creation.
 */
//#define CASTOR_MTX_RECURSIVE

#include <castor/archconfig.h>
#include <pthread_np.h>

typedef struct Mutex {
    alignas(CACHELINE) atomic_uintptr_t		lck;
    atomic_uintptr_t				thr;
} Mutex;

static inline void
Mutex_Init(Mutex *m)
{
    atomic_store(&m->lck, 0UL);
    atomic_store(&m->thr, 0UL);
}

static inline void
Mutex_Lock(Mutex *m)
{
#ifdef CASTOR_MTX_RECURSIVE
    uintptr_t tid = (uintptr_t)pthread_getthreadid_np();

    ASSERT(tid != 0);

    if (atomic_load(&m->thr) == tid) {
	atomic_fetch_add(&m->lck, 1UL);
	return;
    }
#else
    uintptr_t tid = 1;
#endif

    uintptr_t exp = 0;
    while (!atomic_compare_exchange_strong(&m->thr, &exp, tid)) {
	__asm__ volatile("pause\n");
	exp = 0;
    }
    atomic_fetch_add(&m->lck, 1UL);
}

static inline void
Mutex_Unlock(Mutex *m)
{
#ifdef CASTOR_MTX_RECURSIVE
    uintptr_t tid = 1; // (uintptr_t)pthread_getthreadid_np();

    ASSERT(atomic_load(&m->thr) == tid);
#endif
    ASSERT(atomic_load(&m->lck) > 0UL);

    if (atomic_fetch_sub(&m->lck, 1UL) == 1UL)
	atomic_store(&m->thr, 0UL);
}

static inline void
Mutex_Assert(Mutex *m)
{
    ASSERT(atomic_load(&m->lck) > 0UL);
}

#endif /* __CASTOR_MTX_H__ */

