#include <stdio.h>
#include <errno.h>
#include "castor_xlat.h"


void
pretty_print_getresuid_test(RRLogEntry entry)
{
    printf("getresuid() = %d\n", (int)entry.value[0]);
}


int main() {
    printf("EAGAIN = %s\n", xlat_errno(EAGAIN));

}

