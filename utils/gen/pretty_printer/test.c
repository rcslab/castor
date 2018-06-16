#include <stdio.h>
#include <errno.h>
#include "castor_xlat.h"

int main() {
    printf("EAGAIN = %s\n", xlat_errno(EAGAIN));
}

