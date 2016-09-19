
#ifndef __RUNTIME_H__
#define __RUNTIME_H__

void OpenLog(const char *logfile, bool forRecord);
void RecordLog();
void ReplayLog();
int QueueOne();
void DumpLog();

#endif /* __RUNTIME_H__ */

