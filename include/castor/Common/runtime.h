
#ifndef __CASTOR_RUNTIME_H__
#define __CASTOR_RUNTIME_H__

void OpenLog(const char *logfile, uintptr_t RegionSz, uint32_t numEvents, bool forRecord);
void LogDone(int *status);
void RecordLog(pid_t *pid);
void ReplayLog(pid_t *pid);
int QueueOne();
void ResumeDebugWait();
void DumpLog(int thread);
void DumpLogDebug();

#endif /* __CASTOR_RUNTIME_H__ */

