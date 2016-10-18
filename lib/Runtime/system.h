
#ifndef __CASTOR_SYSTEM_H__
#define __CASTOR_SYSTEM_H__

int SystemRead(int fd, void *buf, size_t nbytes);
int SystemWrite(int fd, const void *buf, size_t nbytes);
int SystemGetpid();

#endif /* __CASTOR_SYSTEM_H__ */

