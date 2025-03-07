#include <castor/rrshared.h>

__attribute__((constructor)) void
castor_init()
{
    log_init();
}

void
castor_dummy()
{
}
