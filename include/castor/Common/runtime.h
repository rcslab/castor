
#ifndef __CASTOR_RUNTIME_H__
#define __CASTOR_RUNTIME_H__

void OpenLog(const char *logfile, uintptr_t RegionSz, uint32_t numEvents, bool forRecord);
void LogDone();
void RecordLog();
void ReplayLog();
int QueueOne();
void ResumeDebugWait();
void DumpLog();
void DumpLogDebug();

#endif /* __CASTOR_RUNTIME_H__ */

