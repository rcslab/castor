
#ifndef __EVENTS_H__
#define __EVENTS_H__

int RRMutex_Init(pthread_mutex_t *mtx, pthread_mutexattr_t *attr);
int RRMutex_Destroy(pthread_mutex_t *mtx);
int RRMutex_Lock(pthread_mutex_t *mtx);
int RRMutex_TryLock(pthread_mutex_t *mtx);
int RRMutex_Unlock(pthread_mutex_t *mtx);

#endif /* __EVENTS_H__ */

