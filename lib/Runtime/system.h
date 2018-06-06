
#ifndef __CASTOR_SYSTEM_H__
#define __CASTOR_SYSTEM_H__
#include <sys/syscall.h>
#include <sys/stat.h>

extern int __rr_syscall(int no, ...);
extern off_t __rr_syscall_long(int no, ...);

int SystemRead(int fd, void *buf, size_t nbytes);
int SystemWrite(int fd, const void *buf, size_t nbytes);
int SystemGetpid();

// More code from lib/*.c
_Noreturn void __castor_abort(void);
key_t __castor_ftok(const char *path, int id);
int __castor_stat(const char * restrict path, struct stat * restrict sb);

#endif /* __CASTOR_SYSTEM_H__ */

