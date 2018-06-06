#include <unistd.h>
#include <sys/syscall.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <signal.h>
#include "../system.h"

_Noreturn void __castor_abort(void)
{
    __rr_syscall(SYS_kill, __rr_syscall(SYS_getpid), SIGABRT);
    __unreachable();
}
