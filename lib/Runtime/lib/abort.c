#include <unistd.h>
#include <sys/syscall.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <signal.h>
#include "../system.h"



_Noreturn void abort(void)
{
    __rr_syscall(SYS_kill, -1, SIGABRT);
    __unreachable();
}
