#include <sys/stat.h>
#include "../system.h"

int __castor_stat(const char * restrict path, struct stat * restrict sb)
{
    return __rr_syscall(SYS_freebsd11_stat, path, sb);
}
