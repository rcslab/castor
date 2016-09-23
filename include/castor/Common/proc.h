
#ifndef __CASTOR_PROC_H__
#define __CASTOR_PROC_H__

void PinAgent();
void PinProcess(int maxcpus);
void Sandbox();
int Spawn(bool pinned, int maxcpus, char *const argv[]);

#endif /* __CASTOR_PROC_H__ */

