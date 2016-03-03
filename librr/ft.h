
#ifndef __FT_H__
#define __FT_H__

void RRFT_InitMaster();
void RRFT_InitSlave();
void RRFT_Send(int count, RRLogEntry *evt);
int RRFT_Recv(int count, RRLogEntry *evt);

#endif /* __FT_H__ */

