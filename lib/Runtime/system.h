
#ifndef __CASTOR_SYSTEM_H__
#define __CASTOR_SYSTEM_H__
#include <sys/syscall.h>

extern int __rr_syscall(int no, ...);
extern off_t __rr_syscall_long(int no, ...);

int SystemRead(int fd, void *buf, size_t nbytes);
int SystemWrite(int fd, const void *buf, size_t nbytes);
int SystemGetpid();

#endif /* __CASTOR_SYSTEM_H__ */

