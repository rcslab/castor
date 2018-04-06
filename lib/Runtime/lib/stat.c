#include <sys/stat.h>
#include "../system.h"

int stat(const char * restrict path, struct stat * restrict sb)
{
    return __rr_syscall(SYS_stat, path, sb);
}
