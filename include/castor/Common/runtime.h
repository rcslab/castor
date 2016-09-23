
#ifndef __CASTOR_RUNTIME_H__
#define __CASTOR_RUNTIME_H__

void OpenLog(const char *logfile, bool forRecord);
void LogDone();
void RecordLog();
void ReplayLog();
int QueueOne();
void DumpLog();

#endif /* __CASTOR_RUNTIME_H__ */

