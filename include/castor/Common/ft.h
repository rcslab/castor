
#ifndef __FT_H__
#define __FT_H__

void RRFT_InitMaster();
void RRFT_InitSlave(const char *hostname);
void RRFT_Send(uint64_t count, RRLogEntry *evt);
uint64_t RRFT_Recv(uint64_t count, RRLogEntry *evt);

#endif /* __FT_H__ */

